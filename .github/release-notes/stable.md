> [!IMPORTANT]
> ## Report issues from inside Orchid
> If a game crashes, start Orchid again and pick **Help > Report an Issue on GitHub**. It opens a bug report with your build, system, GPU and driver and the end of the crash log already filled in, and opens the log folder so you can attach the full log of the crashed run.

## Hyrule Warriors: Age of Imprisonment combat crash fix

- **Super Mario Bros. Wonder – Nintendo Switch 2 Edition + Meetup in Bellabel Park** now accepts a custom **1–1000 FPS** target in **Properties > System > Frame rate (FPS, game patch)**. The default is 60 FPS; existing preset settings are migrated automatically. Restart the game after changing the target. Higher targets require sufficient CPU/GPU performance.
- Logs now use the `orchid_` prefix. Each game's latest session is retained in its own file, with title initials and Title-ID (for example, `orchid_TLOZBOTW_01007EF00011E000.txt`). Starting that game again replaces its previous log without removing other games' logs.
- Crash reports use the active game's log, or the most recently written game log after restarting Orchid. Game logs retain startup diagnostics such as the build and host information.

- Fixed excessive GPU buffer readbacks during combat. A previously CPU-read page no longer causes an entire estimated 8 MiB compute write to be copied back at every fence.
- Completed readback buffers are recycled during long frames, and pending readbacks have a bounded producer queue with shutdown cancellation.
- Fixed stale thread-local memory translations when a new memory manager reuses an old manager's address.

### Validation and limitations

- The maintainer tested actual Hyrule combat without the previous hit-triggered hang or crash. Observed dips into the 20 FPS range remain; this is not a measured performance benchmark.
- Local build, targeted regression tests (161 assertions in 8 cases), and scoped mutation checks passed.
- The remaining cross-game gameplay gate was explicitly waived by the maintainer for this release. Bananza reached gameplay, but its full comparison test was not completed.
- The separately reported Kirby allocation crash has not been confirmed fixed.
- Xenoblade Chronicles X: the intro video can freeze, and character creation can hang on a black screen.
