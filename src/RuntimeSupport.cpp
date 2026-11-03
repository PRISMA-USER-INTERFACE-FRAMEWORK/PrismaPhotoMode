#include "RuntimeSupport.h"

#include <REL/VariantID.h>
#include <RE/IDs.h>
#include <REX/FModule.h>

#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <span>

namespace PPM::RuntimeSupport
{
    namespace
    {
        enum class Kind : std::uint8_t
        {
            Function,
            Global
        };

        struct Mapping
        {
            const char* name;
            const REL::VariantID* id;
            Kind kind;
        };

        constexpr REL::VariantID kMainSingleton{ 756304, 2698043, 2698043 };
        constexpr REL::VariantID kToggleFreeCameraMode{ 224913, 2248368, 2248368 };
        constexpr REL::VariantID kSetCameraFOV{ 1348813, 2228973, 2228973 };
        constexpr REL::VariantID kPushHUDMode{ 1321764, 2222443, 2222443 };
        constexpr REL::VariantID kPopHUDMode{ 1495042, 2222444, 2222444 };
        constexpr REL::VariantID kBGSSaveLoadManagerSingleton{ 1247320, 2697802, 2697802 };
        constexpr REL::VariantID kUISingleton{ 548587, 2689028, 4796314 };
        constexpr REL::VariantID kBSTimerSingleton{ 1256126, 2703179, 2703179 };
        constexpr REL::VariantID kQGlobalTimeMultiplier{ 365546, 2666307, 2666307 };
        constexpr REL::VariantID kSetGlobalTimeMultiplier{ 1419977, 2267970, 2267970 };
        constexpr REL::VariantID kCalendarSingleton{ 1444952, 2689092, 4796378 };
        constexpr REL::VariantID kSkySingleton{ 484694, 2192448, 2192448 };
        constexpr REL::VariantID kForceWeather{ 698558, 2208861, 2208861 };
        constexpr REL::VariantID kResetWeather{ 6511, 2208860, 2208860 };
        constexpr REL::VariantID kTESDataHandlerSingleton{ 711558, 2688883, 4796135 };
        constexpr REL::VariantID kDOFTrigger{ 1158048, 2199922, 2199922 };
        constexpr REL::VariantID kIMIStop{ 1298537, 2199897, 2199897 };
        constexpr REL::VariantID kGetCameraPosition{ 1574794, 2248334, 2248334 };
        constexpr REL::VariantID kGetCurrentPickIndex{ 1407033, 2701382, 2701382 };
        constexpr REL::VariantID kGetPickRefs{ 875116, 2701391, 2701391 };
        constexpr REL::VariantID kFilterTrigger{ 179769, 2199906, 2199906 };
        constexpr REL::VariantID kFilterStop{ 217873, 2199909, 2199909 };
        constexpr REL::VariantID kInitiateDoNothingPackage{ 89993, 2229807, 2229807 };
        constexpr REL::VariantID kEndInterruptPackage{ 575188, 2229892, 2229892 };
        constexpr REL::VariantID kSetHeading{ 353571, 2229625, 2229625 };
        constexpr REL::VariantID kStopCurrentIdle{ 434460, 2231705, 2231705 };
        constexpr REL::VariantID kSetupSpecialIdle{ 1446774, 2231704, 2231704 };
        constexpr REL::VariantID kApplyArtObject{ 357908, 2205200, 2205200 };
        constexpr REL::VariantID kApplyEffectShader{ 652173, 2205201, 2205201 };
        constexpr REL::VariantID kGetHandle{ 1573130, 2201196, 2201196 };
        constexpr REL::VariantID kGetDisplayFullName{ 1212056, 2201126, 2201126 };
        constexpr REL::VariantID kImageSpaceManagerSingleton{ 161743, 2712627, 2712627 };
        constexpr REL::VariantID kFreeCameraGetRotation{ 433604, 2248281, 2248281 };
        constexpr REL::VariantID kFromEulerAnglesZXY{ 326460, 2269825, 2269825 };
        constexpr REL::VariantID kChangeAnimFaceArchetype{ 580944, 2230434, 2230434 };
        constexpr REL::VariantID kTypedKeywords{ 1095775, 2692038, 4799330 };
        constexpr std::uintptr_t kFromEulerCallOffset{ 0x1B };
        constexpr std::uint8_t kCallOpcode{ 0xE8 };

        const std::array kCoreMappings{
            Mapping{ "PlayerCamera::ToggleFreeCameraMode", &kToggleFreeCameraMode, Kind::Function },
            Mapping{ "Main::SetCameraFOV", &kSetCameraFOV, Kind::Function },
            Mapping{ "SendHUDMessage::PushHUDMode", &kPushHUDMode, Kind::Function },
            Mapping{ "SendHUDMessage::PopHUDMode", &kPopHUDMode, Kind::Function },
            Mapping{ "Main::Singleton", &kMainSingleton, Kind::Global },
            Mapping{ "BGSSaveLoadManager::Singleton", &kBGSSaveLoadManagerSingleton, Kind::Global },
            Mapping{ "UI::Singleton", &kUISingleton, Kind::Global }
        };

        const std::array kTimeMappings{
            Mapping{ "BSTimer::Singleton", &kBSTimerSingleton, Kind::Global },
            Mapping{ "BSTimer::QGlobalTimeMultiplier", &kQGlobalTimeMultiplier, Kind::Global },
            Mapping{ "BSTimer::SetGlobalTimeMultiplier", &kSetGlobalTimeMultiplier, Kind::Function },
            Mapping{ "Calendar::Singleton", &kCalendarSingleton, Kind::Global },
            Mapping{ "Sky::Singleton", &kSkySingleton, Kind::Function },
            Mapping{ "Sky::ForceWeather", &kForceWeather, Kind::Function },
            Mapping{ "Sky::ResetWeather", &kResetWeather, Kind::Function },
            Mapping{ "TESDataHandler::Singleton", &kTESDataHandlerSingleton, Kind::Global }
        };

        const std::array kDepthOfFieldMappings{
            Mapping{ "ImageSpaceModifierInstanceDOF::Trigger", &kDOFTrigger, Kind::Function },
            Mapping{ "ImageSpaceModifierInstance::Stop", &kIMIStop, Kind::Function },
            Mapping{ "PlayerCamera::GetCameraPosition", &kGetCameraPosition, Kind::Function },
            Mapping{ "Console::GetCurrentPickIndex", &kGetCurrentPickIndex, Kind::Global },
            Mapping{ "Console::GetPickRefs", &kGetPickRefs, Kind::Global }
        };

        bool g_timeControlsAvailable{ false };
        const std::array kFilterMappings{
            Mapping{ "ImageSpaceModifierInstanceForm::Trigger1", &kFilterTrigger, Kind::Function },
            Mapping{ "ImageSpaceModifierInstanceForm::Stop1", &kFilterStop, Kind::Function },
            Mapping{ "TESDataHandler::Singleton", &kTESDataHandlerSingleton, Kind::Global }
        };

        const std::array kCharacterMappings{
            Mapping{ "Actor::InitiateDoNothingPackage", &kInitiateDoNothingPackage, Kind::Function },
            Mapping{ "Actor::EndInterruptPackage", &kEndInterruptPackage, Kind::Function },
            Mapping{ "Actor::SetHeading", &kSetHeading, Kind::Function },
            Mapping{ "AIProcess::StopCurrentIdle", &kStopCurrentIdle, Kind::Function },
            Mapping{ "AIProcess::SetupSpecialIdle", &kSetupSpecialIdle, Kind::Function },
            Mapping{ "TESObjectREFR::ApplyArtObject", &kApplyArtObject, Kind::Function },
            Mapping{ "TESObjectREFR::ApplyEffectShader", &kApplyEffectShader, Kind::Function },
            Mapping{ "TESObjectREFR::GetHandle", &kGetHandle, Kind::Function },
            Mapping{ "TESObjectREFR::GetDisplayFullName", &kGetDisplayFullName, Kind::Function },
            Mapping{ "PlayerCamera::GetCameraPosition", &kGetCameraPosition, Kind::Function },
            Mapping{ "Console::GetCurrentPickIndex", &kGetCurrentPickIndex, Kind::Global },
            Mapping{ "Console::GetPickRefs", &kGetPickRefs, Kind::Global },
            Mapping{ "TESDataHandler::Singleton", &kTESDataHandlerSingleton, Kind::Global }
        };

        const std::array kLightingMappings{
            Mapping{ "ImageSpaceManager::Singleton", &kImageSpaceManagerSingleton, Kind::Global }
        };

        const std::array kViewRollMappings{
            Mapping{ "FreeCameraState::GetRotation", &kFreeCameraGetRotation, Kind::Function },
            Mapping{ "NiMatrix3::FromEulerAnglesZXY", &kFromEulerAnglesZXY, Kind::Function }
        };

        const std::array kExpressionMappings{
            Mapping{ "Actor::ChangeAnimFaceArchetype", &kChangeAnimFaceArchetype, Kind::Function },
            Mapping{ "BGSKeyword::TypedKeywords", &kTypedKeywords, Kind::Global }
        };

        bool g_depthOfFieldAvailable{ false };
        bool g_filtersAvailable{ false };
        bool g_charactersAvailable{ false };
        bool g_lightingAvailable{ false };
        bool g_viewRollAvailable{ false };
        bool g_expressionsAvailable{ false };

        bool ResolveMappings(std::span<const Mapping> a_mappings, Family a_family)
        {
            const auto module = REX::FModule::GetExecutingModule();
            const auto executableVersion = module.GetFileVersion();
            if (Classify(executableVersion) != a_family) {
                spdlog::error("PrismaPhotoMode: executable runtime {} does not match accepted family {}", executableVersion.string(), Name(a_family));
                return false;
            }

            const auto text = module.GetSection(".text");
            const auto textStart = text.GetAddress();
            const auto textEnd = textStart + text.GetSize();
            if (!textStart || textEnd <= textStart) {
                spdlog::error("PrismaPhotoMode: executable .text section could not be resolved");
                return false;
            }
            const auto data = module.GetSection(".data");
            const auto bss = module.GetSection(".bss");

            bool ready = true;
            for (const auto& mapping : a_mappings) {
                spdlog::info("PrismaPhotoMode: resolving required runtime mapping '{}'", mapping.name);

                const auto address = mapping.id->address();
                if (!address) {
                    spdlog::error("PrismaPhotoMode: runtime mapping '{}' resolved to null for {}", mapping.name, executableVersion.string());
                    ready = false;
                    continue;
                }

                const auto inSection = [address](const auto& section) {
                    const auto start = section.GetAddress();
                    return start && address >= start && address < start + section.GetSize();
                };
                const bool valid = mapping.kind == Kind::Function ? inSection(text) : (inSection(data) || inSection(bss));
                if (!valid) {
                    spdlog::error(
                        "PrismaPhotoMode: runtime mapping '{}' resolved outside its executable section at 0x{:X}",
                        mapping.name,
                        address);
                    ready = false;
                }
            }

            if (ready) {
                spdlog::info("PrismaPhotoMode: {} required mappings resolved for {} {}", a_mappings.size(), Name(a_family), executableVersion.string());
            }
            return ready;
        }

        bool FromEulerCallSiteMatches()
        {
            const auto site = kFreeCameraGetRotation.address() + kFromEulerCallOffset;
            const auto* code = reinterpret_cast<const std::uint8_t*>(site);
            if (code[0] != kCallOpcode) {
                spdlog::error("PrismaPhotoMode: FreeCameraState::GetRotation+0x1B is not a call instruction; view roll disabled");
                return false;
            }
            std::int32_t displacement{};
            std::memcpy(&displacement, code + 1, sizeof(displacement));
            const auto target = site + 5 + static_cast<std::uintptr_t>(static_cast<std::intptr_t>(displacement));
            if (target != kFromEulerAnglesZXY.address()) {
                spdlog::error(
                    "PrismaPhotoMode: FreeCameraState::GetRotation+0x1B calls 0x{:X}, not NiMatrix3::FromEulerAnglesZXY; view roll disabled",
                    target);
                return false;
            }
            return true;
        }
    }

    Family Classify(const REL::Version& version) noexcept
    {
        if (version == F4SE::RUNTIME_1_10_163) return Family::OG;
        if (version == F4SE::RUNTIME_1_10_984) return Family::NG;
        if (version >= F4SE::RUNTIME_1_11_137) return Family::AE;
        return Family::Unsupported;
    }

    const char* Name(Family family) noexcept
    {
        switch (family) {
        case Family::OG: return "OG";
        case Family::NG: return "NG";
        case Family::AE: return "AE";
        default: return "UNSUPPORTED";
        }
    }

    bool ValidateMappings(Family family)
    {
        return ResolveMappings(kCoreMappings, family);
    }

    void ValidateTimeMappings(Family family)
    {
        g_timeControlsAvailable = ResolveMappings(kTimeMappings, family);
    }

    bool TimeControlsAvailable() noexcept
    {
        return g_timeControlsAvailable;
    }

    void ValidateDepthOfFieldMappings(Family a_family)
    {
        g_depthOfFieldAvailable = ResolveMappings(kDepthOfFieldMappings, a_family);
    }

    bool DepthOfFieldAvailable() noexcept
    {
        return g_depthOfFieldAvailable;
    }

    void ValidateFilterMappings(Family a_family)
    {
        g_filtersAvailable = ResolveMappings(kFilterMappings, a_family);
    }

    bool FiltersAvailable() noexcept
    {
        return g_filtersAvailable;
    }

    void ValidateCharacterMappings(Family a_family)
    {
        g_charactersAvailable = ResolveMappings(kCharacterMappings, a_family);
    }

    bool CharactersAvailable() noexcept
    {
        return g_charactersAvailable;
    }

    void ValidateLightingMappings(Family a_family)
    {
        g_lightingAvailable = ResolveMappings(kLightingMappings, a_family);
    }

    bool LightingAvailable() noexcept
    {
        return g_lightingAvailable;
    }

    void ValidateViewRollMappings(Family a_family)
    {
        g_viewRollAvailable = ResolveMappings(kViewRollMappings, a_family) && FromEulerCallSiteMatches();
    }

    bool ViewRollAvailable() noexcept
    {
        return g_viewRollAvailable;
    }

    std::uintptr_t ViewRollCallSite() noexcept
    {
        return g_viewRollAvailable ? kFreeCameraGetRotation.address() + kFromEulerCallOffset : 0;
    }

    void ValidateExpressionMappings(Family a_family)
    {
        g_expressionsAvailable = ResolveMappings(kExpressionMappings, a_family);
    }

    bool ExpressionsAvailable() noexcept
    {
        return g_expressionsAvailable;
    }

    void ChangeAnimFaceArchetype(RE::Actor* a_actor, RE::BGSKeyword* a_keyword)
    {
        using Function = void (*)(RE::Actor*, RE::BGSKeyword*);
        reinterpret_cast<Function>(kChangeAnimFaceArchetype.address())(a_actor, a_keyword);
    }
}
