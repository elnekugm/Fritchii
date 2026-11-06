> [!IMPORTANT]
> ## Report crashes from inside Orchid
> If a game crashes, start Orchid again and pick **Help > Report a Crash on GitHub**. It opens a bug report with your build, system, GPU and driver and the end of the crash log already filled in, and opens the log folder so you can attach the full log of the crashed run.

## 0.8.3 — Custom Wonder FPS and per-game logs

- **Super Mario Bros. Wonder – Nintendo Switch 2 Edition + Meetup in Bellabel Park** now accepts a custom **1–1000 FPS** target in **Properties > System > Frame rate (FPS, game patch)**. The default is 60 FPS; existing preset settings are migrated automatically. Restart the game after changing the target. Higher targets require sufficient CPU/GPU performance.
- Logs now use the `orchid_` prefix. Each game's latest session is retained in its own file, with title initials and Title-ID (for example, `orchid_TLOZBOTW_01007EF00011E000.txt`). Starting that game again replaces its previous log without removing other games' logs.
- Crash reports use the active game's log, or the most recently written game log after restarting Orchid. Game logs retain startup diagnostics such as the build and host information.

The Wonder patch still validates the executable build and instruction sites before applying changes. Unsupported executables remain unchanged.
