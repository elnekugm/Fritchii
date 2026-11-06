> [!WARNING]
> **Very experimental.** This build is a performance pass for Mario Tennis Fever that was written
> without being run: the changes were only reviewed and syntax-checked, none of them was measured
> or played. Expect regressions, possibly crashes or graphics errors. Keep your normal build around,
> and please report what you see (with fps before and after, if you can).

## What's new

### Mario Tennis Fever: performance (untested)
- **GPU thread split no longer waits before every command list.** The front only waits for the
  back for command lists on pages the GPU has written, once no new page has been marked for a
  while (before, every GPFIFO entry drained the back; an older note measured the front stalled
  ~40% of the time in this game). `EDEN_NX2_CMD_DRAIN=1` restores the old behaviour.
- **Presents no longer block an emulated CPU core.** The Switch 2 graphics sessions (vi2 present,
  GPU submissions, syncpoints) run on a host thread of their own instead of AM's service thread on
  emulated core 3, which every present used to hold for the GPU fence wait and vblank pacing.
  `EDEN_NX2_GSV_HOST_THREAD=0` restores the old behaviour.
- **Balanced fences (this game's default) got cheaper:** no full GPU wait after DMA copies whose
  data isn't downloaded at fences, empty fences with nothing in flight run their callback at once,
  greater-or-equal semaphore acquires that are already met skip the wait for the split's back
  (`EDEN_NX2_ACQUIRE_PEEK=0` turns this off).
- **New per-game defaults** (your own per-game setting always wins): reactive flushing off (read
  traps take its place) and asynchronous presentation on.
- **Read traps:** a load that once hit one stays on fastmem in the JIT, and slow-path reads of
  untrapped pages no longer wait for the GPU thread.
- **Less work per draw and per frame:** the pipeline is looked up once per draw, texture view and
  sampler ids are kept next to their descriptors, storage buffer sizes come from a per-thread
  cache, no submit after each GPU ASTC decode (`EDEN_NX2_ASTC_FLUSH=1` restores it), the applet
  capture is drawn twice a second instead of every frame.
- **Kernel:** host service threads are woken after the global scheduler lock is released, the lock
  word sits on its own cache line, the timing thread is woken only when needed and gets 1 ns timer
  slack on Linux.

### Fixes found on the way
- Compute inline uploads were collected for the whole session in native mode (memory growth and
  ever larger reallocations on the GPU thread).
- Page kinds of small GPU pages were changed without the memory manager's lock.

- Add-on RomFS packages embedded in a NUP are now registered with the DLC service and
  can be opened by the game. Previously the loader recognized these packages but warned
  that mounting was unsupported.
- Only DLC belonging to the running application is exposed. When a container carries
  several versions of a DLC, the newest usable RomFS is selected.
- Newly packed NUP files preserve the DLC version from the dump directory instead of
  writing every version as zero. Existing archives are left unchanged.
- Authentic NACP control metadata is preserved, including edition launch settings and legacy
  DLC/save ownership. Native edition programs can use their original application's DLC IDs.
  NUP title and version overrides remain separate from those control bytes.
- The Switch 2 font-set request returns bounded descriptors into the built-in font memory.
- Native presentation honors the vertical inversion flag used by Kirby's edition program.
- LayeredFS mods also apply to NUP and other non-updatable RomFS sources.

### Graphics, shaders and settings

- The graphics quality picker remembers settings per game and includes verified profiles
  for additional supported games, including the original Kirby and the Forgotten Land.
- Native uniform shifts, carry/LEA sequences, packed conversions and local-memory
  addressing handle additional captured instruction patterns.
- Vulkan uniform reads follow the actual mapped segments of split buffers and preserve
  holes in sparse backing. Buffer readback can span separate cached buffers.
- Texture upload and vertex setup paths reduce redundant work. Custom graphics settings
  survive global settings reloads.
- Optional bounded diagnostics help attribute host stalls, frame hitches and unmapped
  GPU reads without enabling those traces for normal play.

### Known limitations

- Native editions need their native executable as well as the DLC assets. A NUP containing
  only the legacy executable cannot be turned into the native edition by mounting its DLC.
- Kirby's native edition title screen and DLC delivery were verified with a complete local
  program/data set. Star-Crossed World unlocks after clearing Natural Plains. Native gameplay
  still has texture/sampler rendering errors and unsupported cooperative-matrix shaders;
  the complete expansion has not been validated.
- Older NUPs whose packer wrote every DLC version as zero cannot recover their original
  version ordering automatically; repack from the original dump when needed.
- Kirby Air Riders still has rendering and unsupported compute-shader issues.
- Yoshi shader corrections do not establish full game compatibility.
- See the performance and compatibility table in the README for game-specific status.

The PGO profile is unchanged from 0.7.5. Ported fixes retain their source credits in the commits.
