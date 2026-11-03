#pragma once

#include <REL/Version.h>
#include <F4SE/Version.h>

#include <cstdint>

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
}
