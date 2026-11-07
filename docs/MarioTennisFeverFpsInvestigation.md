# Mario Tennis Fever FPS investigation

Status: experimental test build, not a completed frame-rate-independent patch.

The user's reports are 120 FPS running too fast in the menu and 30 FPS running
too slowly in gameplay, including after restarting the new high-FPS build.

## Confirmed paths

Offsets below refer to the unpacked, build-checked game NRO.

- UI root `0x11ce260` defaults to a fixed step of 1 at `0x11ce2c4`.
  Its animation update bypasses the shared simulation scale. Scale this default
  by `60 / target_fps`; preserve the branch that already reads the shared scale.
- Character integration reads `model+0x390` at `0x106ed28`. It is quantized to
  1/64-frame units. Character animation also uses this field.
- Ball simulator `0x1081b40` converts the shared scale to 1/64-frame units.
  At `0x1081bd8`, W23 is the requested advance. The live state is
  NRO base + `0x3e4b9e0`. Stack copies used for prediction intentionally have a
  different clock and must not be changed indiscriminately.
- The ball loop consumes multiple substeps; a requested step of 2 is not
  unconditionally capped to 1. Collisions and inactive states can stop it early.

## Runtime evidence, 2026-11-07

Private Orchid MCP GPU runs, separate profiles and displays:

- `20261107-140503-a45a9844`: target 30, practice court rendered. Character input
  step 2 at about 29.5 calls/s, clock speed about 0.98. Live-ball requested step
  2 at about 29.8 calls/s, clock speed about 0.99. UI settles near 1.0.
- `20261107-140812-0ea06842`: target 120, title/menu and practice court rendered.
  Actual rate about 100–102 FPS. UI, character and ball input clocks about
  0.83–0.85. The fixed target-based scaling does not keep real-time speed when
  presentation misses its target. This is an unresolved limitation.
- The second run stopped cleanly. The first ended while its original development
  executable was being rebuilt; its incomplete teardown cannot establish a
  separate game crash. Later runs use an immutable executable copy.

Logs live under
`~/.local/share/codex-orchid-mcp/state/runs/<run-id>/profile/data/eden/log/`.

## Logging and remaining checks

The former audit of the shared scale did not measure the independent UI clock
and must not be called a whole-game speed measurement. The replacement logs
separate UI, character and live-ball input clocks. Ball results explicitly say
`ball_flight_requested`; they do not prove all requested physics was executed.
CSV also records character/live-ball positions before integration.

The planned held-stick distance comparison at 30 versus 60 FPS is unfinished:
the next GPU boot was rejected because another Orchid MCP instance owned a GPU
test. Do not bypass that ownership or claim the comparison passed. No position
comparison has yet validated the final CSV additions at runtime.

Other known limits: character and ball quantization introduces error at targets
such as 144 or 1000 FPS; script frame counters and per-call damping need further
validation. A complete real-time correction must cover these consumers and
preserve pause, hit stop, replay and prediction semantics.

Validation: local GUI build passed; 18 Tennis/Wonder test cases, 12,646
assertions passed; mutation check on selected patch-header lines killed 16
mutants, survived 0, invalid 6. Mutation report:
`/home/leon/Orchid/orchid-issues/mutation/20261107-140216-1228319`.
The tests validate patch guards, bytes and arithmetic, not every SVC register or
the game's physical movement.
