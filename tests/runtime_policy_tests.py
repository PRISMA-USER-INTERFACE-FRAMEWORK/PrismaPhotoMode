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
    "Main::Singleton",
    "BSTimer::Singleton",
    "BSTimer::QGlobalTimeMultiplier",
    "BSTimer::SetGlobalTimeMultiplier",
    "Calendar::Singleton",
    "Sky::Singleton",
    "Sky::ForceWeather",
    "Sky::ResetWeather",
    "TESDataHandler::Singleton",
    "BGSSaveLoadManager::Singleton",
    "UI::Singleton",
    "ImageSpaceModifierInstanceDOF::Trigger",
    "ImageSpaceModifierInstance::Stop",
    "PlayerCamera::GetCameraPosition",
    "Console::GetCurrentPickIndex",
    "Console::GetPickRefs",
    "ImageSpaceModifierInstanceForm::Trigger1",
    "ImageSpaceModifierInstanceForm::Stop1",
    "Actor::InitiateDoNothingPackage",
    "Actor::EndInterruptPackage",
    "Actor::SetHeading",
    "AIProcess::StopCurrentIdle",
    "AIProcess::SetupSpecialIdle",
    "TESObjectREFR::ApplyArtObject",
    "TESObjectREFR::ApplyEffectShader",
    "TESObjectREFR::GetHandle",
    "TESObjectREFR::GetDisplayFullName",
    "ImageSpaceManager::Singleton",
    "FreeCameraState::GetRotation",
    "NiMatrix3::FromEulerAnglesZXY",
    "Actor::ChangeAnimFaceArchetype",
    "BGSKeyword::TypedKeywords",
):
    assert f'"{name}"' in runtime, f"RuntimeSupport.cpp must validate {name}"

for path in sorted(src.glob("*.cpp")) + sorted(src.glob("*.h")):
    if path.name == "RuntimeSupport.cpp":
        continue
    assert "RE::ID::" not in path.read_text(encoding="utf-8"), f"{path.name} must reach engine ids through RuntimeSupport"

for version in ("F4SE::RUNTIME_1_10_163", "F4SE::RUNTIME_1_11_137"):
    assert version in runtime, f"RuntimeSupport.cpp must classify {version}"

assert "family == Family::NG" not in runtime, "RuntimeSupport.cpp must not reject NG"
for triple in (
    "756304, 2698043, 2698043",
    "224913, 2248368, 2248368",
    "1348813, 2228973, 2228973",
    "1321764, 2222443, 2222443",
    "1495042, 2222444, 2222444",
    "1247320, 2697802, 2697802",
    "548587, 2689028, 4796314",
    "1256126, 2703179, 2703179",
    "365546, 2666307, 2666307",
    "1419977, 2267970, 2267970",
    "1444952, 2689092, 4796378",
    "484694, 2192448, 2192448",
    "698558, 2208861, 2208861",
    "6511, 2208860, 2208860",
    "711558, 2688883, 4796135",
    "1158048, 2199922, 2199922",
    "1298537, 2199897, 2199897",
    "1574794, 2248334, 2248334",
    "1407033, 2701382, 2701382",
    "875116, 2701391, 2701391",
    "179769, 2199906, 2199906",
    "217873, 2199909, 2199909",
    "89993, 2229807, 2229807",
    "575188, 2229892, 2229892",
    "353571, 2229625, 2229625",
    "434460, 2231705, 2231705",
    "1446774, 2231704, 2231704",
    "357908, 2205200, 2205200",
    "652173, 2205201, 2205201",
    "1573130, 2201196, 2201196",
    "1212056, 2201126, 2201126",
    "161743, 2712627, 2712627",
    "433604, 2248281, 2248281",
    "326460, 2269825, 2269825",
    "580944, 2230434, 2230434",
    "1095775, 2692038, 4799330",
):
    assert re.search(rf"REL::VariantID k\w+\{{ {re.escape(triple)} \}}", runtime), f"RuntimeSupport.cpp must carry the verified NG id triple {triple}"

assert "catch (" not in runtime and "catch (...)" not in runtime, "RuntimeSupport.cpp must not swallow exceptions"
assert "resolving required runtime mapping" in runtime
assert "inSection(data) || inSection(bss)" in runtime, "singleton mappings must resolve in .data or .bss"
assert "Kind::Function ? inSection(text)" in runtime, "function mappings must resolve in .text"

FUNCTION_MAPPINGS = (
    "PlayerCamera::ToggleFreeCameraMode",
    "Main::SetCameraFOV",
    "SendHUDMessage::PushHUDMode",
    "SendHUDMessage::PopHUDMode",
    "BSTimer::SetGlobalTimeMultiplier",
    "Sky::Singleton",
    "Sky::ForceWeather",
    "Sky::ResetWeather",
    "ImageSpaceModifierInstanceDOF::Trigger",
    "ImageSpaceModifierInstance::Stop",
    "PlayerCamera::GetCameraPosition",
    "ImageSpaceModifierInstanceForm::Trigger1",
    "ImageSpaceModifierInstanceForm::Stop1",
    "Actor::InitiateDoNothingPackage",
    "Actor::EndInterruptPackage",
    "Actor::SetHeading",
    "AIProcess::StopCurrentIdle",
    "AIProcess::SetupSpecialIdle",
    "TESObjectREFR::ApplyArtObject",
    "TESObjectREFR::ApplyEffectShader",
    "TESObjectREFR::GetHandle",
    "TESObjectREFR::GetDisplayFullName",
    "FreeCameraState::GetRotation",
    "NiMatrix3::FromEulerAnglesZXY",
    "Actor::ChangeAnimFaceArchetype",
)
GLOBAL_MAPPINGS = (
    "Main::Singleton",
    "BGSSaveLoadManager::Singleton",
    "UI::Singleton",
    "BSTimer::Singleton",
    "BSTimer::QGlobalTimeMultiplier",
    "Calendar::Singleton",
    "TESDataHandler::Singleton",
    "Console::GetCurrentPickIndex",
    "Console::GetPickRefs",
    "ImageSpaceManager::Singleton",
    "BGSKeyword::TypedKeywords",
)
mapping_kinds = re.findall(r'Mapping\{ "([^"]+)", &\w+, Kind::(Function|Global) \}', runtime)
assert mapping_kinds, "RuntimeSupport.cpp must declare Mapping entries with a Kind"
for mapping_name, kind in mapping_kinds:
    expected = "Function" if mapping_name in FUNCTION_MAPPINGS else "Global" if mapping_name in GLOBAL_MAPPINGS else None
    assert kind == expected, f"{mapping_name} must be Kind::{expected}, found Kind::{kind}"
assert {name for name, _ in mapping_kinds} == set(FUNCTION_MAPPINGS) | set(GLOBAL_MAPPINGS), "every RuntimeSupport mapping must be pinned to a Kind"
assert "enum class Family" in runtime_header
assert "ValidateMappings" in main and "ValidateMappings(g_runtimeFamily)" in main
assert "ValidateTimeMappings(g_runtimeFamily)" in main, "main.cpp must gate the time controls through ValidateTimeMappings"
assert "ValidateDepthOfFieldMappings(g_runtimeFamily)" in main, "main.cpp must gate depth of field through ValidateDepthOfFieldMappings"
assert "ValidateFilterMappings(g_runtimeFamily)" in main, "main.cpp must gate filters through ValidateFilterMappings"
assert "ValidateCharacterMappings(g_runtimeFamily)" in main, "main.cpp must gate characters through ValidateCharacterMappings"
assert "ValidateLightingMappings(g_runtimeFamily)" in main, "main.cpp must gate lighting through ValidateLightingMappings"
assert "ValidateViewRollMappings(g_runtimeFamily)" in main, "main.cpp must gate the view roll through ValidateViewRollMappings"
assert "InstallViewRoll()" in main, "main.cpp must install the view roll hook at kGameDataReady"
assert "ValidateExpressionMappings(g_runtimeFamily)" in main, "main.cpp must gate expressions through ValidateExpressionMappings"
assert "REL::GetTrampoline().write_call<5>" in (src / "PhotoMode.cpp").read_text(encoding="utf-8"), "view roll must patch a 5-byte call through the trampoline"

print("PrismaPhotoMode runtime and PrismaUI policy tests passed")
