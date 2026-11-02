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

Details, and the switch for each change: `NX2_NOTES.md`, section "Mario Tennis Fever performance
pass".
