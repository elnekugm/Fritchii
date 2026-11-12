# Selevet Eglantine

An experimental, unofficial Windows build of the Eglantine Nintendo Switch 2 emulator, tuned for Mario Kart World on a Ryzen 5 5500 with an RTX 3050 8 GB and 16 GB of RAM.

It is based on [zortos293/Eglantine build 31](https://github.com/zortos293/Eglantine/releases/tag/nx2-build31-20261101) (commit `badc91ebce`). That project builds on [Djihads80/Eglantine](https://github.com/Djihads80/Eglantine), Eden and yuzu. All credit for the emulator goes to those projects. This is not an official release of any of them, so please don't report problems with it to them.

## Download

Get the zip from the Releases page, extract it into its own folder, and run `eglantine.exe`. Settings are already built in, so you only need to add your keys and firmware and map your controller.

**You need:**

- A CPU that supports Zen 3 instructions. This build is compiled with `-march=znver3`, so it may crash on start on older AMD CPUs and many Intel CPUs. If that happens, use the official build 31 or build from source with your own flags.
- The latest [Microsoft Visual C++ x64 runtime](https://aka.ms/vs/17/release/vc_redist.x64.exe).
- Your own legally dumped keys, firmware and games. None are included.

## What's different from build 31

| Change | Files | Notes |
|---|---|---|
| GPU mapping table updates only compare the changed range | `nx2_gpu_mapping_table.h` | Matched the original over 2 million randomised operations, and ran about 160-240x faster per operation in a benchmark. That is not an fps figure. |
| Async draw check uses the current draw's real count | `draw_manager.cpp`, `maxwell_3d.h`, `vk_pipeline_cache.cpp` | The old check could read a stale count from the buffer the draw doesn't use. `EDEN_NX2_STALE_DRAW_COUNTS=1` restores it for comparison. |
| Logs draws waiting on a pipeline build | `vk_graphics_pipeline.cpp` | Diagnostics only. |
| Saves the PGO profile every 30 s in training builds | `src/yuzu/main.cpp` | Only compiled with `EGLANTINE_PGO_GENERATE`. |
| Command entries only wait when their page was GPU-written before | `dma_pusher.cpp` | No measurable fps change in repeated no-input tests. A page's first write can show stale commands for one frame. `EDEN_NX2_COMMAND_DRAIN=1` restores the old wait. |
| Tested settings are the defaults | `src/common/settings.h`, `src/core/game_settings.*` | Controller mappings aren't included. Booted to rendered frames with a default config and no input; a full race has not been run on this exact build. |

The release is also built with Clang 23.1.4, ThinLTO, Zen 3 tuning and a profile trained on about 7 minutes of Mario Kart World.

## Launch flags

`nx2_switches.txt` beside the program sets `EDEN_NX2_UNSAFE_PREEMPTIVE_READS=1`, an unsafe shortcut that build 31's own README does not recommend by default. Delete that line if you get glitches. Unlike build 31's launcher, this build leaves the split GPU thread on, because the project's notes measured it at 1.46x throughput.

## Built-in settings

These are the program's defaults, in `src/common/settings.h`. They apply to all games, and Unsafe accuracy can break some Switch 1 titles, so change them in the emulator's settings or per game if something misbehaves. If you already have an Eden or Eglantine config, any settings you changed yourself stay as they are.

| Setting | Value |
|---|---|
| GPU mode | Fast |
| DMA accuracy | Unsafe |
| GPU fence behaviour | Balanced (Immediate for Mario Kart World) |
| Scaling filter | ScaleForce |
| Memory layout | 8 GB (Switch 2 games always use 12 GB) |
| CPU accuracy | Unsafe |
| Asynchronous shaders | On |
| Asynchronous presentation | On |
| Disk shader cache | On |
| Driver pipeline cache | Off |
| Sync memory operations | Off |
| Reactive flushing | Off |
| VRAM usage mode | Aggressive |
| Resolution | 0.5x, handheld mode |

Turning sync memory operations off gave a black screen entering races on an older Eglantine build. It didn't on this one in testing, but if you get a black screen, turn it back on first.

## Test results

Tested on one PC only: Ryzen 5 5500, RTX 3050 8 GB, 16 GB RAM at 3600 MHz, Mario Kart World.

| Run | Median fps | Lowest 600-frame average |
|---|---|---|
| This build, played at 1x resolution | 55.5 | 25.9 |
| v1, sync memory on, 0.5x | 36.7 | 23.5 |
| v1, sync memory off, 0.5x | 42 | 24.2 |

The sync memory off run was short, so treat it as promising, not final. Heavy tracks can still drop while textures load, and this is not a locked 60 fps.

## Known problems

- The game can log a panic (for example 2162-0001 or 2010-0212) and may keep running in a broken state. The cause isn't fully known.
- Stopping a game can leave `eglantine.exe` running with no window, holding a lot of RAM. Check Task Manager before relaunching.
- Don't share your shader cache: `vulkan.bin` contains game code.

## Building from source

You need Visual Studio with the C++ tools, Clang 23, `glslangValidator` and `patch` on `PATH`. Edit the paths in the cache files in this folder to match your machine.

```bash
cmake -C tuned/pgo-use-cache.cmake -S . -B build/pgo-use -G Ninja
cmake --build build/pgo-use --target yuzu
```

To train your own profile, build with `tuned/pgo-gen-cache.cmake`, run with `LLVM_PROFILE_FILE` set, play for a while, then merge the output with `llvm-profdata merge -o tuned/eglantine.profdata`. A profile trained on your own games and CPU should suit you better than the one included here.

## 30FPS PATCH

The 30FPS PATCH release limits the game to 30 fps. It uses this source with `tuned/nx2_switches-30fps.txt` as `nx2_switches.txt` beside the exe. `EDEN_NX2_PRESENT_HZ=<rate>` sets the limit; without it, the game is held to 60 as usual.

## Build 104 merge

The source now also includes the changes from [Djihads80/Eglantine build-104](https://github.com/Djihads80/Eglantine/releases/tag/build-104). Sparse texture tile mapping (`nx2:gpu 216`) is included but off by default, because it ran about 15-20% slower on an RTX 3050. `EDEN_NX2_SPARSE_REMAP=1` in `nx2_switches.txt` turns it on.

## Performance build

This source sends work to the host GPU every 128 draws instead of every 4096 (`EDEN_NX2_FLUSH_DRAWS`), skips waits on semaphores already met in memory (`EDEN_NX2_ACQUIRE_PEEK=0` restores them), runs at high process priority (`EDEN_NX2_HIGH_PRIORITY=0` turns that off) and uses a profile retrained on Mario Kart World. The semaphore changes and the benchmark logging (`EDEN_BENCH_LOG`, `EDEN_TAS_AUTOSTART`) come from [shintasku/selevet-eglantine-linux](https://github.com/shintasku/selevet-eglantine-linux).

## Performance build 2

This source fetches GPU-written command lists in one batch per GPU wait instead of one wait each (`EDEN_NX2_CMD_BATCH=0` restores that), and skips the wait for a semaphore when a release that meets it is already queued for the split GPU thread (`EDEN_NX2_ACQUIRE_QUEUED=0` restores that). The release turns on `EDEN_NX2_QMD_ACQUIRE_SKIP=1` in `nx2_switches.txt`. Changing the memory layout now restarts the emulated system. `EDEN_NX2_CMD_STATS=1` logs the GPU waits every 10 seconds, and `EDEN_NX2_SHADOW_SKIP=N` (off) redraws shadow maps every N frames.

## Performance build 3

The CPU translator no longer uses AVX-512, even on CPUs that have it (`EDEN_NX2_AVX512=1` turns it back on). The upscaler fallback also looks for its images at constant buffer 0 +0x60 and +0x70, where Donkey Kong Bananza version 458752 keeps them. The video decoder only marks a picture from the final input as the end of a video. The release no longer turns on `EDEN_NX2_QMD_ACQUIRE_SKIP`.

## Performance build 4

The loader makes the Switch 2 SDK's `nn::vi::detail::DefaultDisplay::QueryVrrSupported` report VRR support, found by its exported name (`EDEN_NX2_REPORT_VRR=0` turns it off). Switch 2 draws skip a pipeline that is still being built instead of waiting for it when asynchronous shaders are on (`EDEN_NX2_ASYNC_SKIP=0`). `EDEN_NX2_VBLANK_ALIGN=1` holds late frames to the next 60 Hz vblank. A debug readback that froze the game once is off by default.

## Performance build 5

Performance build 4 merged with Djihads80/Eglantine build-131: mesh and task shaders (`VK_EXT_mesh_shader`), vi2 native events and display queries, shared device ranges, cold memory hints, the camera session, the NUP loader and its Fire Emblem: Fortune's Weave fixes. NVIDIA optical flow frame generation from anormalhorse's `nvof` branch is a new frame generation choice (RTX 20 series or newer, untested on hardware). Selevet's shader translator is kept, now with build-131's ISBERD/ISBEWR for mesh programs, UPOPC, loop exits to a BSYNC in programs that also return, and absolute-value constant-buffer F2I. Three of build-131's defaults are reverted: ASTC recompression is Uncompressed again (BC3 darkened Mario Kart World's ground), a swap interval above 1 is honoured only once a game keeps it for 120 presents (Mario Kart World switches between 1 and 2 and fell from about 40 to 20 fps), and GPU-written buffers are copied back for Pokemon Pokopia from its program id at load.

## Performance build 7: The Legend of Zelda: Ocarina of Time

Performance build 5 merged with zortos293/Lichii build-67 and its Ocarina of Time branches, plus ports from ducky's and Kajcsi's Ocarina work (via zortos293/Lichii and Frizti/Lichii), so Ocarina of Time boots and plays. The program is now `selevet.exe` and the window title says Lichii. Kirby Air Riders' fixes from build 6 are not in this build: keep build 6 for that game.

- **Boots and renders:** the boot abort (svc 0x34 wait type 3) and the black world (depth-compare gathers in the final pass) are fixed.
- **Flicker fixed:** old frames no longer flash between new ones. A CPU write to a command-list page the GPU had written was held back until the next fence while the GPU thread kept running the page's old copy, a whole earlier frame. The write now marks the copy stale at once and is applied before the page is flushed (`EDEN_NX2_LIST_CPU_SYNC=0` restores the old behaviour). From Kajcsi: the occlusion counter is paused instead of reset at each submission (one-frame white flashes) and kept bounded.
- **Full resolution:** the game's dynamic resolution followed emulated GPU time and sat at 854x480. zortos293's built-in Max Resolution patch keeps it at full scale, so only the emulator's Resolution setting changes it.
- **Built-in Dynamic FPS:** the community Dynamic-fps patch is built in: the game steps by the real time since the last frame, so it keeps its speed when frames take longer.
- Both built-in patches show in Add-Ons as "Max Resolution (built-in)" and "Dynamic FPS (built-in)", on by default, apply only to the matching game build, and step aside for a user mod that patches the same place.
- **Shaders:** native Ampere forms that were rejected now translate, so their draws and dispatches run: FFMA with an immediate on shadowed registers (white leaf-shaped holes in the foliage), LEA carries into any uniform predicate, two-source F2IP.U8 byte packing, indexed ULDC.U8/U16, and P2R between a call's link setup and the CALL. From zortos293 and ducky: the 1080p upscaler gathers, the door NaN, the vegetation pointer loads, the sky ambient buffer and the pointer-table pools.
- **Speed:** the split GPU thread waits only for writes that overlap a macro's memory read (`EDEN_NX2_DMA_RANGES`), stops re-waiting on command pages already seen CPU-written (`EDEN_NX2_LIST_TRUST`), skips acquires already met (`EDEN_NX2_ACQUIRE_PEEK`) or met by a release still queued for its second thread (`EDEN_NX2_ACQUIRE_QUEUED`), and keeps fast draws across per-object occlusion queries and report registers. From zortos293 and ducky: the fast draw paths, uploads instead of downloads for CPU writes on GPU pages (#103), fence downloads of only the needed pages (#104), a cached page-table walk, a short spin before idling CPU cores and bounded written-storage marks.
- **Haze and glare:** a temporary fog, exposure and bloom reduction for Ocarina of Time (`EDEN_NX2_OOT_*` in `nx2_switches.txt`). It hides the over-bright haze; the cause isn't fixed yet.
- **CPU switches:** `EDEN_CPU_ACCURATE_FLOAT`, `EDEN_CPU_FUSED_FMA`, `EDEN_CPU_GLOBAL_MONITOR` and `EDEN_CPU_FASTMEM_CHECK` turn single Unsafe/Auto CPU shortcuts off. The release turns the first two on.
- **Diagnostics:** `EDEN_NX2_FRAME_DUMP_COUNT=n` captures n consecutive frames; `EDEN_NX2_SHADER_DUMP` also keeps rejected compute programs.

**Known problems in Ocarina of Time:**

- **The framerate isn't perfect yet:** about 26-28 fps in Kokiri Forest on a Ryzen 5 5500 with an RTX 3050, lower in the busiest views. The emulator's GPU threads are the limit, not the graphics card. Work on it continues.
- Some shader stutter the first time new areas load.

## Performance build 8: Ocarina of Time speed-ups (fix 25)

Performance build 7 plus speed-ups ported from Frizti/Lichii's frizti-v2 branch, AMD shader fixes from MythosOfs/Orchid, and a wider check for the built-in patches.

- **Fewer GPU thread stalls (999sian):** the split GPU thread waits for work on events instead of spinning, which frees a CPU core, and Ocarina of Time presents one frame ahead instead of waiting for each frame's GPU fence. The one-frame-ahead present adds a frame of input latency; `EDEN_NX2_FRAMES_AHEAD=0` in `nx2_switches.txt` turns it off.
- **Bigger GPU submissions (999sian):** up to 4096 draws per native submission, with fragment barriers deferred across unrelated draws.
- **Cheaper timing (999sian):** an invariant-TSC calibrated host clock and an inlined guest counter read.
- **Uniform uploads (Matt Artist):** native programs' 64 KiB uniform banks upload without page protection churn.
- **Memory lookups (zortos293):** continuous-range answers come from cached page runs, keyed by 4 KiB page because Ocarina of Time shares big pages between allocations.
- **Deferred work (chuppatricky, Dr. Sergio):** deferred submissions go out before waiting on pending fences and fence callbacks, so those waits no longer stall on work that was never sent.
- **Split writers (Frizti):** only compute and DMA methods that write memory count as writers for the split GPU thread's waits.
- **Built-in patches:** a mismatched hook or code area is logged instead of skipping the patch, and a version mismatch names both build ids, so Dynamic FPS and Max Resolution apply to more dumps of the same game version.
- **AMD (MythosOfs/Orchid):** nested returns are counted before demotes move to the end of a fragment shader (a black scene on AMD), the AMDVLK shuffle workaround needs 32-wide subgroups, and mesh and task shaders are left out of it.
- **Events:** unsignalled MWAITX wakes retry until the event's deadline (zortos293).
- **Diagnostics:** the log names each compute shader the first time it runs and every GPU wait of half a second or more; `EDEN_NX2_SKIP_COMPUTE=hash,hash` skips listed compute shaders for testing.

## License and AI disclosure

GPL-3.0-or-later, the same as the projects it's based on. See `LICENSE.txt`.

The changes in this repo were written with Claude (Anthropic), then tested in-game by the repo owner. They have not been reviewed by the Eglantine developers.
