#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace PPM::Screenshot
{
    inline bool InText(std::uintptr_t address, std::size_t length, std::uintptr_t begin, std::uintptr_t end) noexcept
    {
        return begin && end > begin && address >= begin && address <= end && length <= end - address;
    }

    inline bool BranchMatches(std::uintptr_t site, std::uint8_t opcode, std::uintptr_t target,
                              std::uintptr_t begin, std::uintptr_t end) noexcept
    {
        if (!InText(site, 5, begin, end) || !InText(target, 1, begin, end)) return false;
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(site);
        if (bytes[0] != opcode) return false;
        std::int32_t displacement = 0;
        std::memcpy(&displacement, bytes + 1, sizeof(displacement));
        return site + 5 + static_cast<std::intptr_t>(displacement) == target;
    }

    inline bool LayoutMatches(std::uintptr_t saver, std::uintptr_t manager, std::uintptr_t renderer,
                              bool oldGeneration, std::uintptr_t begin, std::uintptr_t end) noexcept
    {
        constexpr std::array<std::uint8_t, 8> oldMap{ 0x42, 0x8B, 0x94, 0x91, 0xC4, 0x0D, 0, 0 };
        constexpr std::array<std::uint8_t, 8> newMap{ 0x42, 0x8B, 0x94, 0x91, 0xF4, 0x0D, 0, 0 };
        constexpr std::array<std::uint8_t, 4> multiply{ 0x48, 0x8D, 0x14, 0x40 };
        constexpr std::array<std::uint8_t, 11> texture{ 0x48, 0x03, 0xD2, 0x48, 0x8B, 0x9C, 0xD1, 0x68, 0x0A, 0, 0 };
        const auto& map = oldGeneration ? oldMap : newMap;
        const auto mapOffset = oldGeneration ? 8U : 12U;
        const auto multiplyOffset = oldGeneration ? 32U : 35U;
        if (!InText(saver, 0x96, begin, end) || !InText(manager, 32, begin, end) ||
            !InText(renderer, 50, begin, end)) return false;
        return BranchMatches(saver + 0x91, 0xE8, manager, begin, end) &&
               BranchMatches(manager + 0x1B, 0xE9, renderer, begin, end) &&
               std::memcmp(reinterpret_cast<const void*>(manager + mapOffset), map.data(), map.size()) == 0 &&
               std::memcmp(reinterpret_cast<const void*>(renderer + multiplyOffset), multiply.data(), multiply.size()) == 0 &&
               std::memcmp(reinterpret_cast<const void*>(renderer + 39), texture.data(), texture.size()) == 0;
    }
}
