> [!IMPORTANT]
> ## Report crashes from inside Orchid
> If a game crashes, start Orchid again and pick **Help > Report a Crash on GitHub**. It opens a bug report with your build, system, GPU and driver and the end of the crash log already filled in, and opens the log folder so you can attach the full log of the crashed run. Please use it for every crash: reports with a log get fixed much faster, and reports without one are closed.

## 0.8.1 — Hotfix for Wonder Switch 2 Edition loading

Fixes the loading hang in **Super Mario Bros. Wonder – Nintendo Switch 2 Edition +
Meetup in Bellabel Park** caused by the missing `pl:u::OpenFont` service (command 2100).
The game could stop loading or fault at an unmapped address after that request in 0.8.0.

- Implements the font service returned by `OpenFont`, including single-font and font-set
  descriptors backed by the loaded built-in fonts.
- Copies font data into the game's transfer memory, validates the handle and buffer size,
  and keeps the memory alive until the font service closes.
- Verified locally with an edition NUP containing the native program and upgrade data:
  the game passes loading and reaches the character selection with Rosalina and Co-Star Luma.

This hotfix does not convert a Switch 1 dump into a Switch 2 Edition dump. The NUP must
contain the edition executable and its required upgrade resources. Later levels and all
Bellabel Park content have not been exhaustively tested.
