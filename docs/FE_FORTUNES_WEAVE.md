# Fire Emblem: Fortune's Weave (`04007EF01F8AE000`) — bring-up notes

Base: `nx2-20261031` (`60ef6d275`), branch `fe-fortunes-weave`. Linux build (CachyOS,
GCC 16, radv 26.99.99 / RX 9070 XT), `eden-cli`, loose ExeFS dump with update `131072` (2.0.0).

## State (2026-10-31)

Boots and runs at 60 fps, but the picture stays black: the native (Ampere) shader lowering
rejects the shaders the game dispatches, so every draw is skipped (`Draw skipped (no pipeline)`)
and the compute dispatches are dropped (`Native QMD failed: native shader dispatch required`).
The game logic itself keeps running (~56 presents/s, vblank-paced, no hangs, no crashes).

## How to run a loose dump from the CLI

`eden-cli` opens `<path>/main`, so the path must be the **exefs directory**, not the game folder:

```
SDL_VIDEODRIVER=wayland WAYLAND_DISPLAY=wayland-1 \
  ./eden-cli -g "/home/leon/Switch2/Fire Emblem Fortune's Weave/updates/131072/exefs"
```

The update's `romfs/` is found next to that `exefs/` directory. Log: `~/.local/share/eden/log/eden_log.txt`.

## Blockers, in order of first hit

`lower_to_maxwell.h` fails on the first offending instruction of a program, so later modes
surface only once the earlier ones are lowered.

Lowered already (2026-10-31, this branch): `ISETP.EX` (64-bit compare high word), `REDUX.MIN`,
`SHF` with a constant source (`0xa19`).

Still rejected, with the instruction that fails:

Still rejected (fail-fast: the next one only surfaces once this one is lowered). The three
graphics-path ones keep the screen black; the rest are compute passes:

| Message | Instruction |
|---|---|
| native surface descriptor buffer conflict | `b9b 00007b9b 8080480c 00100f00` (2 programs) |
| native formatted surface store mode | `b9b 02007b9b 60005804 0010af00`, `b9b 1200ab9b 6000aa04 0010af00` |
| unsupported native opcode (TXQ.B, bindless handle) | `370 1c1d7370 08000000 000001ff` |
| native integer multiply mode | `824 9a2b7824 00000001 008e0603` |
| unsupported native opcode (IMAD.WIDE, cbuf) | `625 34028625 00005e00 078e0203` |
| unsupported native opcode | `226 47407226 00000040 000010ff` |
| unsupported native opcode (F2I.64) | `311 00047311 00000004 0021d800` |
| unsupported native opcode | `fae 020d7fae 00000000 0e101c48` |
| native integer carry/third source | `c10 0c0a7c10 00000004 0ff9e0ff` |
| native constant add carry mode | `a10 2c2c8a10 00005c00 07b5e005` |
| native matrix multiply | `23c ...` — `EDEN_NX2_TENSOR=1` does not clear it |

The two `b9b` classes are not a missing case: `ShaderEnvironment::texture_bound` holds one
descriptor buffer per program, so a program that stores through a surface and samples a
texture that live in different descriptor buffers has nowhere to put the second one. That,
and `TXQ.B` on a bindless handle (a descriptor in a register pair instead of a bank offset),
are the two structural items; the remaining opcodes are ordinary case work.
Also seen, not yet shader related:

- `Unmapped Device ReadBlock` bursts — guest reads inside ranges that never got mapped.
- `nx2:gsv` TIPC `11` unimplemented (falls back to the auto stub).
- `Load: Failed to read title for ROM` — no control NACP in a loose dump, expected.

## What the nxnative repo says about this game (for reference)

`git.eden-emu.dev/penis6/nxnative-penis6`, profile `dm/games/04007EF01F8AE000`. That project is
not an emulator — it recompiles the game's own binaries and rebuilds Switch graphics on Metal —
so its patches do not transfer, but its game profile does:

- 2.0.0 / `131072`, SDK 22.2.0, modules `main subsdk0 subsdk1 subsdk2 sdk`, NVN2.
- Shaders live in RomFS as `.bfsha.zst` / `.ptcl.zst`: 16796 of 16809 programs convert offline.
- Memory: reserve `0x1400000000`, alias `0xC00000000`, stack `0x10000000`, app pool `0x263000000`;
  the game asks for a 36 GB allocator at boot.
- Movies are AV1; there the host's decoder needed the CPU-buffer path, forced by patching the
  movie player in `main`. Not relevant to an emulator, which runs the console path.
- DLSS goes through a temporal scaler.

The same SASS semantics keep biting both projects (FSEL source mods, FSET with a uniform,
IMAD.HI's 64-bit addend, F2I.U64's high word, calls returning into their block, jump tables in
`c1`) — ours already lower all of these; the modes above are the ones neither has yet.

## Still rejected (2026-11-01, 20 compute dispatches)

| Family | Instruction words | What is missing |
|---|---|---|
| IDP.4A.S8.S8 | `226 3f367226 00000036 000010ff`, `226 42797226 00000039 000010ff`, `226 47407226 00000040 000010ff` | signed byte dot product (3 programs) |
| LEA.HI with a uniform source | `c11 00057c11 00000004 0f8fd0ff`, `c11 0a0a7c11 00000004 0f8fe0ff` | uniform base in the high-word path |
| LDGSTS.E.BYPASS.128 | `fae 020d7fae 00000000 0e101c48` | LDG.128 + STS.128 split |
| I2F variant | `312 00027312 00000002 00301000` | opcode 0x312 has no case yet |
| MUFU operation 9 | `308 000c7308 0000000c 00002400` | SM86 MUFU op 9 |
| reserved register as a source | `211 f73b7211 00000000 078f18ff`, `831 d88d7831 3a003a00 0000008d` | programs whose own operands use R246-R254 |

Getting a picture needs every one of these; the display buffer (0x91f41000, 3840x2160) is written by compute passes only.

## Still rejected (2026-11-01, 12 compute dispatches, 6 instructions)

| Family | Instruction words | What is missing |
|---|---|---|
| reserved GPR as a source | `211 f73b7211 00000000 078f18ff`, `831 d88d7831 3a003a00 0000008d` | the program's own operands live in R216-R254, where the lowering keeps uniforms and scratch: needs register remapping |
| MUFU operation 9 | `308 000c7308 0000000c 00002400` | SM86 MUFU op 9 |
| LDGSTS.E.BYPASS.128 | `fae 020d7fae 00000000 0e101c48` | LDG.128 + STS.128 split |
| F2I variant 0x312 | `312 00027312 00000002 00301000` | opcode 0x312 has no case yet |
| CALL with another link setup | `944 00007944 00002d00 03c00000` | the preceding move is not the `0x7802` form the lowering expects |

Done since the last table: IADD3 with one or two carry predicates and a third-source carry-in, LEA.HI.X with a constant base, LEA.HI with a uniform base, HFMA2.RELU with a predicate output, USHF over UR0-UR21, IDP.4A.S8.S8.
