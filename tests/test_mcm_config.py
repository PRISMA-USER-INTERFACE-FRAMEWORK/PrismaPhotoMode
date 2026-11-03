import configparser
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
mcm = root / "MCM/Config/PrismaPhotoMode"
page = json.loads((mcm / "config.json").read_text(encoding="utf-8"))
defaults = configparser.ConfigParser()
defaults.read(mcm / "settings.ini", encoding="utf-8")
legacy = configparser.ConfigParser()
legacy.read(root / "config/PrismaPhotoMode.ini", encoding="utf-8")

controls = {item["id"]: item for item in page["content"] if "id" in item}
for key in ("ToggleKey", "FocusKey", "PanelScale"):
    control = controls[f"i{key}:General"]
    assert control["valueOptions"]["sourceType"] == "ModSettingInt"
    value = defaults.getint("General", f"i{key}")
    assert value == legacy.getint("General", key)
    assert control["valueOptions"]["min"] <= value <= control["valueOptions"]["max"]

pad = controls["iFocusPadButton:General"]
assert pad["valueOptions"]["sourceType"] == "ModSettingInt"
assert pad["valueOptions"]["options"][defaults.getint("General", "iFocusPadButton")] == "Back"
assert legacy.getint("General", "FocusPadButton") == 65568
buttons = {
    "D-pad up": "kDPAD_Up", "D-pad down": "kDPAD_Down", "D-pad left": "kDPAD_Left",
    "D-pad right": "kDPAD_Right", "Start": "kStart", "Back": "kBack",
    "Left stick": "kLStick", "Right stick": "kRStick", "Left shoulder": "kLShoulder",
    "Right shoulder": "kRShoulder", "A": "kAButton", "B": "kBButton",
    "X": "kXButton", "Y": "kYButton", "Left trigger": "kLTrigger", "Right trigger": "kRTrigger",
}
source = (root / "src/main.cpp").read_text(encoding="utf-8")
mapping = re.search(r"constexpr std::array kFocusPadButtons\s*\{(.*?)\};", source, re.S)
assert mapping
symbols = re.findall(r"RE::BS_BUTTON_CODE::(\w+)", mapping.group(1))
assert pad["valueOptions"]["options"] == ["Disabled", *buttons]
assert symbols == list(buttons.values())

print("PrismaPhotoMode MCM configuration checks PASS")
