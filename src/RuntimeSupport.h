#pragma once

#include <REL/Version.h>
#include <F4SE/Version.h>

#include <cstdint>

namespace RE
{
    class Actor;
    class BGSKeyword;
}

namespace PPM::RuntimeSupport
{
    enum class Family : std::uint8_t
    {
        Unsupported = 0,
        OG,
        NG,
        AE
    };

    [[nodiscard]] Family Classify(const REL::Version& version) noexcept;
    [[nodiscard]] const char* Name(Family family) noexcept;
    [[nodiscard]] bool ValidateMappings(Family family);
    void ValidateTimeMappings(Family family);
    [[nodiscard]] bool TimeControlsAvailable() noexcept;
    void ValidateDepthOfFieldMappings(Family family);
    [[nodiscard]] bool DepthOfFieldAvailable() noexcept;
    void ValidateFilterMappings(Family family);
    [[nodiscard]] bool FiltersAvailable() noexcept;
    void ValidateCharacterMappings(Family family);
    [[nodiscard]] bool CharactersAvailable() noexcept;
    void ValidateLightingMappings(Family family);
    [[nodiscard]] bool LightingAvailable() noexcept;
    void ValidateViewRollMappings(Family family);
    [[nodiscard]] bool ViewRollAvailable() noexcept;
    [[nodiscard]] std::uintptr_t ViewRollCallSite() noexcept;
    void ValidateExpressionMappings(Family family);
    [[nodiscard]] bool ExpressionsAvailable() noexcept;
    void ChangeAnimFaceArchetype(RE::Actor* a_actor, RE::BGSKeyword* a_keyword);
}
