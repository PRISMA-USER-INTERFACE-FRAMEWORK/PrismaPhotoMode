"""Static policy guards for PrismaPhotoMode runtime and PrismaUI contracts. Reads files only."""

import os
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
src = root / "src"
runtime = (src / "RuntimeSupport.cpp").read_text(encoding="utf-8")
runtime_header = (src / "RuntimeSupport.h").read_text(encoding="utf-8")
main = (src / "main.cpp").read_text(encoding="utf-8")

framework = os.environ.get("PRISMA_UI_FRAMEWORK_ROOT")
if not framework:
    raise SystemExit("Set PRISMA_UI_FRAMEWORK_ROOT to a Fallout-4-Prisma-UI-Framework checkout")
canonical_header = Path(framework) / "src/PrismaUI_F4_API.h"
if not canonical_header.is_file():
    raise SystemExit(f"Missing canonical provider header: {canonical_header}")
if not (src / "PrismaUI_F4_API.h").is_file():
    raise SystemExit("Missing src/PrismaUI_F4_API.h: copy the canonical provider header")
if (src / "PrismaUI_F4_API.h").read_bytes() != canonical_header.read_bytes():
    raise SystemExit("src/PrismaUI_F4_API.h differs from the canonical provider header")

pin = (root / "deps/prisma-ui-framework.ref").read_text(encoding="utf-8").strip()
assert re.fullmatch(r"[0-9a-f]{40}", pin), "deps/prisma-ui-framework.ref must be a full commit hash"
pin = (root / "deps/commonlibf4.ref").read_text(encoding="utf-8").strip()
assert re.fullmatch(r"[0-9a-f]{40}", pin), "deps/commonlibf4.ref must be a full commit hash"

for path in sorted(src.glob("*.cpp")) + sorted(src.glob("*.h")):
    if path.name in ("RuntimeSupport.cpp", "RuntimeSupport.h", "PrismaUI_F4_API.h"):
        continue
    text = path.read_text(encoding="utf-8")
    raw_id = re.search(r"REL::(ID|VariantID|Offset|Relocation)\s*[{(]\s*\d", text)
    assert raw_id is None, f"{path.name} must not hard-code a relocation id; use RuntimeSupport mappings"

for name in (
    "PlayerCamera::ToggleFreeCameraMode",
    "Main::SetCameraFOV",
    "SendHUDMessage::PushHUDMode",
    "SendHUDMessage::PopHUDMode",
):
    assert f'"{name}"' in runtime, f"RuntimeSupport.cpp must validate {name}"

for version in ("F4SE::RUNTIME_1_10_163", "F4SE::RUNTIME_1_11_137"):
    assert version in runtime, f"RuntimeSupport.cpp must classify {version}"

assert "NG unverified" in runtime, "RuntimeSupport.cpp must reject NG until its ids are verified"
assert runtime.index("family == Family::NG") < runtime.index("->address()"), "the NG rejection must precede every address() call"

assert "catch (" not in runtime and "catch (...)" not in runtime, "RuntimeSupport.cpp must not swallow exceptions"
assert "resolving required runtime mapping" in runtime
assert "address < textStart || address >= textEnd" in runtime
assert "enum class Family" in runtime_header
assert "ValidateMappings" in main and "ValidateMappings(g_runtimeFamily)" in main

print("PrismaPhotoMode runtime and PrismaUI policy tests passed")
