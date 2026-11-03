#include "RuntimeSupport.h"

#include <REL/VariantID.h>
#include <RE/IDs.h>
#include <REX/FModule.h>

#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>
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

        const std::array kCoreMappings{
            Mapping{ "PlayerCamera::ToggleFreeCameraMode", &kToggleFreeCameraMode, Kind::Function },
            Mapping{ "Main::SetCameraFOV", &kSetCameraFOV, Kind::Function },
            Mapping{ "SendHUDMessage::PushHUDMode", &kPushHUDMode, Kind::Function },
            Mapping{ "SendHUDMessage::PopHUDMode", &kPopHUDMode, Kind::Function },
            Mapping{ "Main::Singleton", &kMainSingleton, Kind::Global }
        };

        const std::array kTimeMappings{
            Mapping{ "BSTimer::Singleton", &RE::ID::BSTimer::Singleton, Kind::Global },
            Mapping{ "BSTimer::QGlobalTimeMultiplier", &RE::ID::BSTimer::QGlobalTimeMultiplier, Kind::Global },
            Mapping{ "BSTimer::SetGlobalTimeMultiplier", &RE::ID::BSTimer::SetGlobalTimeMultiplier, Kind::Function },
            Mapping{ "Calendar::Singleton", &RE::ID::Calendar::Singleton, Kind::Global },
            Mapping{ "Sky::Singleton", &RE::ID::Sky::Singleton, Kind::Global },
            Mapping{ "Sky::ForceWeather", &RE::ID::Sky::ForceWeather, Kind::Function },
            Mapping{ "Sky::ResetWeather", &RE::ID::Sky::ResetWeather, Kind::Function },
            Mapping{ "TESDataHandler::Singleton", &RE::ID::TESDataHandler::Singleton, Kind::Global }
        };

        bool g_timeControlsAvailable{ false };

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
        if (family == Family::NG) {
            spdlog::error("PrismaPhotoMode: NG 1.10.984 has no verified BGSSaveLoadManager id; photo mode disabled");
            return false;
        }
        return ResolveMappings(kCoreMappings, family);
    }

    void ValidateTimeMappings(Family family)
    {
        if (family == Family::NG) {
            spdlog::warn("PrismaPhotoMode: time controls disabled on NG; BSTimer, Sky, Calendar and TESDataHandler ids are not verified for 1.10.984");
            g_timeControlsAvailable = false;
            return;
        }
        g_timeControlsAvailable = ResolveMappings(kTimeMappings, family);
    }

    bool TimeControlsAvailable() noexcept
    {
        return g_timeControlsAvailable;
    }
}
