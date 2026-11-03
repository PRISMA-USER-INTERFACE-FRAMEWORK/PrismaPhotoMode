# PrismaPhotoMode

PrismaPhotoMode is a Fallout 4 F4SE plugin that adds a photo mode with a PrismaUI web panel instead of ImGui.

Based on PhotoModeF4 by powerof3 (MIT). See `THIRD_PARTY_NOTICES`.

## Installation

Install the F4SE version and Address Library data matching your Fallout 4 runtime, and install PrismaUI_F4 with the V10 API. The pinned framework revision is listed in `deps/prisma-ui-framework.ref`.

Download the `PrismaPhotoMode` CI artifact from a successful run and extract `PrismaPhotoMode.zip` with a mod manager, or copy its contents into the game's `Data` directory. The archive contains the DLL and INI under `F4SE/Plugins` and the panel under `PrismaUI_F4/views/PrismaPhotoMode`. Start the game through F4SE.

## What it does

- A hotkey (default F9) enters and leaves photo mode in gameplay only.
- The pause menu has a Photo Mode entry when the panel is ready. It is disabled while the Pip-Boy is open.
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
- The screenshot action hides the panel and grid. When a frame is selected, photo mode blends it into the game image and writes a PNG under `Documents/My Games/Fallout4/Photos`; if framed capture fails, the game screenshot is used instead. Other web overlays are not included in the framed capture.

## Controls

- F9 (`ToggleKey`) enters or leaves photo mode.
- Tab (`FocusKey`) gives the panel the mouse and keyboard. Tab or Escape releases them so the camera gets input again.
- Back (`FocusPadButton`) gives or releases the panel on a gamepad. In the panel, the D-pad moves, A activates, LB and RB switch tabs, and B releases.

## Configuration

`config/PrismaPhotoMode.ini` ships to `Data/F4SE/Plugins/PrismaPhotoMode.ini`.
With Mod Configuration Menu installed, the same settings are available under Prisma Photo Mode. MCM saves changed values in `Data/MCM/Settings/PrismaPhotoMode.ini`; these override the plugin INI and take effect after closing the pause menu.
When upgrading an existing custom plugin INI, MCM initially displays its own defaults until each setting is changed in MCM. To use an MCM default instead of a custom plugin INI value, select another value first and then select the default.

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

- The Windows DLL is built in CI. It has not been tested on a game install.
- Supported runtimes: Fallout 4 1.10.163 (OG), 1.10.984 (NG) and 1.11.137 or later (AE).
- Mouse passthrough while the panel is unfocused is not implemented. Keyboard and mouse go to the panel while it has focus.

## Layout

- `src/`: plugin source. `PrismaUI_F4_API.h` is a verbatim copy of the provider header from Fallout-4-Prisma-UI-Framework.
- `assets/views/PrismaPhotoMode/`: the web panel (`index.html`, `style.css`, `app.js`).
- `config/`: the INI file.
- `deps/`: pinned commits for the framework header (`prisma-ui-framework.ref`) and CommonLibF4 (`commonlibf4.ref`).
- `tests/`: static checks, run with `python -X utf8 tests/run_checks.py` (needs `PRISMA_UI_FRAMEWORK_ROOT` pointing at a Fallout-4-Prisma-UI-Framework checkout).
- `tools/ci.py`: CI stages, `checks` and `dll`. `.github/workflows/ci.yml` runs them on GitHub-hosted runners.
