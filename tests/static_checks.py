"""Static asset contract checks for the PrismaPhotoMode Prisma view. Reads files only."""

from collections import Counter
from html.parser import HTMLParser
from pathlib import Path
import re
import sys

REQUIRED_IDS = (
    "fov", "speed", "focus-btn", "focus-state", "panel", "show-panel",
    "hide-panel-btn", "screenshot-btn", "reset-btn", "reset-effects-btn", "exit-btn",
    "fx-grid-toggle", "fx-vignette-toggle", "vignette-strength",
    "fx-letterbox-toggle", "letterbox-aspect", "fx-tint-toggle", "tint-color", "tint-opacity",
    "fx-grain-toggle", "grain-amount", "fx-frame-toggle", "frame-width",
    "fx-grid", "fx-vignette", "fx-tint", "fx-grain", "fx-frame",
    "fx-letterbox-top", "fx-letterbox-bottom", "fx-letterbox-left", "fx-letterbox-right",
    "frame-category", "frame-name", "texture-category", "texture-name", "fx-overlay", "fx-texture", "grid-kind", "time-freeze", "timemult", "timemult-out", "hour", "hour-out", "timescale", "timescale-out",
    "weather-select", "weather-clear-btn", "panel-scale", "panel-scale-out",
    "fx-dof-toggle", "dof-distance", "dof-distance-out", "dof-strength", "dof-strength-out", "dof-range", "dof-range-out", "dof-auto",
    "filter-select", "filter-strength", "filter-strength-out",
    "brightness", "brightness-out", "saturation", "saturation-out", "contrast", "contrast-out", "lighting-reset-btn",
    "show-player", "roll", "roll-out",
    "pick-name", "freeze-btn", "release-btn", "face-btn", "idle-target", "idle-category", "idle-select", "idle-btn", "expression-target", "expression-select", "expression-btn", "art-select", "art-btn", "shader-select", "shader-btn",
)
NATIVE_COMMANDS = ("fov", "speed", "roll", "focus", "screenshot", "reset", "exit", "freeze", "timemult", "hour", "timescale", "weather", "lists", "dof", "dofdist", "dofstrength", "dofrange", "autofocus", "brightness", "saturation", "contrast", "lightreset", "showplayer", "filter", "filterstrength", "pick", "pickfreeze", "face", "idle", "playeridle", "expression", "playerexpression", "art", "shader")
KEPT_OVERLAYS = ("#fx-vignette", "#fx-tint", "#fx-grain", "#fx-frame", "#fx-letterbox")


class IdCollector(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = []
        self.hrefs = []

    def handle_starttag(self, tag, attrs):
        for key, value in attrs:
            if key == "id" and value:
                self.ids.append(value)
            if key in ("href", "src") and value and not value.startswith(("http", "data:")):
                self.hrefs.append(value)


def check(root: Path) -> None:
    views = root / "assets/views/PrismaPhotoMode"
    html = (views / "index.html").read_text(encoding="utf-8")
    collector = IdCollector()
    collector.feed(html)
    duplicates = sorted(value for value, count in Counter(collector.ids).items() if count > 1)
    if duplicates:
        raise SystemExit(f"index.html duplicate DOM ids: {', '.join(duplicates)}")
    for ref in collector.hrefs:
        if not (views / ref).is_file():
            raise SystemExit(f"index.html missing asset reference: {ref}")
    for control in REQUIRED_IDS:
        if f'id="{control}"' not in html:
            raise SystemExit(f"missing static control: {control}")

    app = (views / "app.js").read_text(encoding="utf-8")
    style = (views / "style.css").read_text(encoding="utf-8")
    if "window.pmSetState" not in app:
        raise SystemExit("app.js must expose window.pmSetState")
    if "window.pmSetOverlays" not in app:
        raise SystemExit("app.js must expose window.pmSetOverlays")
    if "window.pmSetLists" not in app:
        raise SystemExit("app.js must expose window.pmSetLists")
    if "window.pmSetFilters" not in app:
        raise SystemExit("app.js must expose window.pmSetFilters")
    if "window.pmSetCharacters" not in app or "window.pmSetPick" not in app:
        raise SystemExit("app.js must expose window.pmSetCharacters and window.pmSetPick")
    if '"pm_command"' not in app:
        raise SystemExit("app.js must call the pm_command listener")

    used = set(re.findall(r'["`]([a-z]+)\|', app))
    unknown = sorted(used - set(NATIVE_COMMANDS))
    if unknown:
        raise SystemExit(f"app.js sends commands the native side does not handle: {', '.join(unknown)}")

    for selectors, body in re.findall(r"([^{}]+)\{([^{}]*)\}", style):
        if "body.shooting" not in selectors:
            continue
        for overlay in KEPT_OVERLAYS:
            if overlay in selectors:
                raise SystemExit(f"style.css shooting rule must not hide overlay {overlay}")
        if "#fx-grid" not in selectors or "#panel" not in selectors:
            raise SystemExit("style.css shooting rule must hide #panel and #fx-grid")

    main = (root / "src/main.cpp").read_text(encoding="utf-8")
    handled = re.findall(r'name == "([a-z]+)"', main)
    missing = sorted(set(NATIVE_COMMANDS) - set(handled))
    if missing:
        raise SystemExit(f"main.cpp does not handle commands: {', '.join(missing)}")
    repeated = sorted(name for name, count in Counter(handled).items() if count > 1)
    if repeated:
        raise SystemExit(f"main.cpp matches command names more than once: {', '.join(repeated)}")
    if 'kCommandListener[] = "pm_command"' not in main or 'kStateCallback[] = "pmSetState"' not in main:
        raise SystemExit("main.cpp listener and state names must match app.js")
    if 'kFiltersCallback[] = "pmSetFilters"' not in main:
        raise SystemExit("main.cpp filters callback name must match app.js")
    if 'kCharactersCallback[] = "pmSetCharacters"' not in main or 'kPickCallback[] = "pmSetPick"' not in main:
        raise SystemExit("main.cpp characters and pick callback names must match app.js")


if __name__ == "__main__":
    target = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
    check(target)
    print("PrismaPhotoMode static asset checks PASS")
