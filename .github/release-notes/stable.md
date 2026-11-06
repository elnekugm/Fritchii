> [!IMPORTANT]
> ## Report crashes from inside Orchid
> If a game crashes, start Orchid again and pick **Help > Report a Crash on GitHub**. It opens a bug report with your build, system, GPU and driver and the end of the crash log already filled in, and opens the log folder so you can attach the full log of the crashed run. Please use it for every crash: reports with a log get fixed much faster, and reports without one are closed.

## 0.8.2 — Wonder FPS selection

**Super Mario Bros. Wonder – Nintendo Switch 2 Edition + Meetup in Bellabel Park**
now has a dedicated frame-rate selector in **Properties > System**:
**30, 60, 90, 120, 180 or 240 FPS**, with **60 FPS as the default**.

- Patches the simulation rate and present interval together, keeping the game at its
  intended speed instead of doubling its speed when forcing 60 FPS presentation alone.
- Matches presentation pacing to the selected target. Above 60 FPS, supported Vulkan
  presentation modes allow rendering ahead of the display instead of applying its VSync cap.
- Shows the selector only for Wonder's native edition program, `040015100B514001`.
- Shows the existing **Lock to 30 FPS** checkbox only in **Donkey Kong Bananza** properties.
- Handles the earlier standalone **Wonder 60 FPS normal speed** test mod so it cannot
  override the new selector.

Stop the game before changing the setting, then start it again. Higher targets require
sufficient CPU/GPU performance; the selector does not guarantee that every scene reaches
that target. The built-in Wonder patch validates the NSO build ID and all three instruction
sites, and leaves unsupported executables unchanged.

Validated with a local build, 18 focused test cases (2,071 assertions), and short Wonder
startup checks at 60 and 240 FPS. Extended gameplay at every target has not been tested.
The Wonder loading fix from 0.8.1 remains included.
