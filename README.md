# PrismaPhotoMode

PrismaPhotoMode is a Fallout 4 F4SE plugin that adds a photo mode with a PrismaUI web panel instead of ImGui.

Based on PhotoModeF4 by powerof3 (MIT). See `THIRD_PARTY_NOTICES`.

## What it does

- A hotkey (default F9) enters and leaves photo mode in gameplay only.
- On entry the free camera turns on, game time freezes, the HUD hides, and the photo panel shows.
- On exit the camera, time, HUD and field of view are restored to their values from before entry.
- The panel has a field-of-view slider, a camera speed slider, and buttons for focus, screenshot, reset and exit.
- Visual effects are CSS overlays in the panel view: vignette, letterbox with an aspect picker, colour tint, film grain, frame, and a rule-of-thirds grid.
- The screenshot action hides the panel and grid and keeps the overlays. Whether the engine screenshot captures the web overlays is not verified in game.

## Controls

- F9 (`ToggleKey`) enters or leaves photo mode.
- Tab (`FocusKey`) gives the panel the mouse and keyboard. Tab or Escape releases them so the camera gets input again.

## Configuration

`config/PrismaPhotoMode.ini` ships to `Data/F4SE/Plugins/PrismaPhotoMode.ini`.

- `ToggleKey`: Windows virtual-key code for entering and leaving photo mode. Default 120 (F9).
- `FocusKey`: virtual-key code for focus. Default 9 (Tab).

## Status

- Not tested in game. The plugin has not been built or run on a game install.
- Supported runtimes: Fallout 4 1.10.163 (OG) and 1.11.137 or later (AE). The 1.10.984 (NG) runtime is rejected until its ids are verified, so photo mode stays off there.
- Mouse passthrough while the panel is unfocused is not implemented. Keyboard and mouse go to the panel while it has focus.

## Layout

- `src/`: plugin source. `PrismaUI_F4_API.h` is a verbatim copy of the provider header from Fallout-4-Prisma-UI-Framework.
- `assets/views/PrismaPhotoMode/`: the web panel (`index.html`, `style.css`, `app.js`).
- `config/`: the INI file.
- `deps/`: pinned commits for the framework header (`prisma-ui-framework.ref`) and CommonLibF4 (`commonlibf4.ref`).
- `tests/`: static checks, run with `python -X utf8 tests/run_checks.py` (needs `PRISMA_UI_FRAMEWORK_ROOT` pointing at a Fallout-4-Prisma-UI-Framework checkout).
- `tools/ci.py`: CI stages, `checks` and `dll`. `.github/workflows/ci.yml` runs them on GitHub-hosted runners.

## Credits

Engine-driving logic was ported from PhotoModeF4 by powerof3 (MIT). The license text is in `THIRD_PARTY_NOTICES`.
