#include "RuntimeSupport.h"

#include <REL/VariantID.h>
#include <RE/IDs.h>
#include <REX/FModule.h>

#include <spdlog/spdlog.h>

#include <array>

namespace PPM::RuntimeSupport
{
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
            spdlog::error("PrismaPhotoMode: NG unverified; no verified 1.10.984 ids, photo mode disabled");
            return false;
        }

        const auto module = REX::FModule::GetExecutingModule();
        const auto executableVersion = module.GetFileVersion();
        if (Classify(executableVersion) != family) {
            spdlog::error("PrismaPhotoMode: executable runtime {} does not match accepted family {}", executableVersion.string(), Name(family));
            return false;
        }

        const auto text = module.GetSection(".text");
        const auto textStart = text.GetAddress();
        const auto textEnd = textStart + text.GetSize();
        if (!textStart || textEnd <= textStart) {
            spdlog::error("PrismaPhotoMode: executable .text section could not be resolved");
            return false;
        }

        // Two-id VariantIDs fill the NG slot with the AE id, so NG is rejected before any address().
        struct Mapping
        {
            const char* name;
            const REL::VariantID* id;
        };

        const std::array mappings{
            Mapping{ "PlayerCamera::ToggleFreeCameraMode", &RE::ID::PlayerCamera::ToggleFreeCameraMode },
            Mapping{ "Main::SetCameraFOV", &RE::ID::Main::SetCameraFOV },
            Mapping{ "SendHUDMessage::PushHUDMode", &RE::ID::SendHUDMessage::PushHUDMode },
            Mapping{ "SendHUDMessage::PopHUDMode", &RE::ID::SendHUDMessage::PopHUDMode }
        };

        bool ready = true;
        for (const auto& mapping : mappings) {
            spdlog::info("PrismaPhotoMode: resolving required runtime mapping '{}'", mapping.name);

            const auto address = mapping.id->address();
            if (!address) {
                spdlog::error("PrismaPhotoMode: runtime mapping '{}' resolved to null for {}", mapping.name, executableVersion.string());
                ready = false;
                continue;
            }
            if (address < textStart || address >= textEnd) {
                spdlog::error(
                    "PrismaPhotoMode: runtime mapping '{}' resolved outside executable .text at 0x{:X}",
                    mapping.name,
                    address);
                ready = false;
            }
        }

        if (ready) {
            spdlog::info("PrismaPhotoMode: {} required mappings resolved for {} {}", mappings.size(), Name(family), executableVersion.string());
        }
        return ready;
    }
}
