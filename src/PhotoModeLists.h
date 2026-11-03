#pragma once

#include <string>

namespace PPM::PhotoModeLists
{
    [[nodiscard]] std::string Build();
    [[nodiscard]] std::string BuildOverlays();
    [[nodiscard]] std::string BuildFilters();
    [[nodiscard]] std::string BuildCharacterForms();
    [[nodiscard]] std::string BuildPick(const char* a_name);
}
