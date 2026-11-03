# PrismaPhotoMode

PrismaPhotoMode is a Fallout 4 F4SE plugin that adds a photo mode with a PrismaUI web panel instead of ImGui.

Based on PhotoModeF4 by powerof3 (MIT). See `THIRD_PARTY_NOTICES`.

## What it does

- A hotkey (default F9) enters and leaves photo mode in gameplay only.
- On entry the free camera turns on, game time freezes, the HUD hides, and the photo panel shows.
- On exit the camera, time, HUD and field of view are restored to their values from before entry.
- The panel has a field-of-view slider, a camera speed slider, and buttons for focus, screenshot, reset and exit.
- The Camera tab has a View roll slider under field of view (-90 to 90 degrees, default 0). It tilts the view while photo mode is open. Reset camera and exit set it back to 0. Not verified in game.
- Depth of field has focus distance, strength and range sliders. Auto focus follows the picked reference (the console pick) and uses the manual distance when nothing is picked.
- The Filters tab applies a game image space modifier (chosen from a list) with a strength slider. Not verified in game.
- The Filters tab also has Lighting sliders (brightness, saturation, contrast, 0 to 3). They override the game's image space base data until photo mode closes. Not verified in game.
- The Characters tab acts on the console-picked reference: freeze and release, face the camera, play an idle, and apply an art object or effect shader. Idle target is Picked (the player when nothing is picked) or Player, and the idle list can be filtered by category (the editor ID prefix before the first underscore, or Other). The Expression list applies an AnimFaceArchetype keyword to the same target. Only one actor keeps an expression at a time; choosing None, choosing another actor, or exiting photo mode clears it. Not verified in game.
- Show player hides the player model while photo mode is open. It is shown again when photo mode closes. Not verified in game.
- Visual effects are CSS overlays in the panel view: vignette, letterbox with an aspect picker, colour tint, film grain, frame, and a rule-of-thirds grid.
- The screenshot action hides the panel and grid and keeps the overlays. Whether the engine screenshot captures the web overlays is not verified in game.

## Controls

- F9 (`ToggleKey`) enters or leaves photo mode.
- Tab (`FocusKey`) gives the panel the mouse and keyboard. Tab or Escape releases them so the camera gets input again.
- Back (`FocusPadButton`) gives or releases the panel on a gamepad. In the panel, the D-pad moves, A activates, LB and RB switch tabs, and B releases.

## Configuration

`config/PrismaPhotoMode.ini` ships to `Data/F4SE/Plugins/PrismaPhotoMode.ini`.

- `ToggleKey`: Windows virtual-key code for entering and leaving photo mode. Default 120 (F9).
- `FocusKey`: virtual-key code for focus. Default 9 (Tab).
- `FocusPadButton`: gamepad button code for focus. Default 65568 (Back, 0x10020). 0 disables it.
- `PanelScale`: panel size in percent, 50 to 200. Default 100. The Access tab changes it until photo mode closes.

## Overlays

Frames and textures are PNG images in two folders:

- `Data/PrismaUI_F4/views/PrismaPhotoMode/overlays/frames/<category>/*.png`
- `Data/PrismaUI_F4/views/PrismaPhotoMode/overlays/textures/<category>/*.png`

Each subfolder is a category. The Overlays tab has a Frames section and a Textures section, each with its own Category and item select. Textures are drawn under frames. Only `.png` files directly inside a category folder are listed. A missing folder gives an empty list.

## Status

- Not tested in game. The plugin has not been built or run on a game install.
- Supported runtimes: Fallout 4 1.10.163 (OG), 1.10.984 (NG) and 1.11.137 or later (AE).
- Mouse passthrough while the panel is unfocused is not implemented. Keyboard and mouse go to the panel while it has focus.

## Layout

- `src/`: plugin source. `PrismaUI_F4_API.h` is a verbatim copy of the provider header from Fallout-4-Prisma-UI-Framework.
- `assets/views/PrismaPhotoMode/`: the web panel (`index.html`, `style.css`, `app.js`).
- `config/`: the INI file.
- `deps/`: pinned commits for the framework header (`prisma-ui-framework.ref`) and CommonLibF4 (`commonlibf4.ref`).
- `tests/`: static checks, run with `python -X utf8 tests/run_checks.py` (needs `PRISMA_UI_FRAMEWORK_ROOT` pointing at a Fallout-4-Prisma-UI-Framework checkout).
- `tools/ci.py`: CI stages, `checks` and `dll`. `.github/workflows/ci.yml` runs them on GitHub-hosted runners.
