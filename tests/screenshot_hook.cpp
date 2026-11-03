#include "ScreenshotHook.h"
#include <array>
#include <cstdint>
#include <cstring>

int main()
{
    std::array<std::uint8_t, 512> code{};
    const auto begin = reinterpret_cast<std::uintptr_t>(code.data());
    const auto end = begin + code.size();
    const auto saver = begin;
    const auto manager = begin + 0x100;
    const auto renderer = begin + 0x180;
    const auto branch = [](std::uintptr_t site, std::uint8_t opcode, std::uintptr_t target) {
        auto* bytes = reinterpret_cast<std::uint8_t*>(site);
        bytes[0] = opcode;
        const auto displacement = static_cast<std::int32_t>(static_cast<std::intptr_t>(target) - static_cast<std::intptr_t>(site + 5));
        std::memcpy(bytes + 1, &displacement, sizeof(displacement));
    };
    branch(saver + 0x91, 0xE8, manager);
    branch(manager + 0x1B, 0xE9, renderer);
    const std::array<std::uint8_t, 4> multiply{ 0x48, 0x8D, 0x14, 0x40 };
    const std::array<std::uint8_t, 11> texture{ 0x48, 0x03, 0xD2, 0x48, 0x8B, 0x9C, 0xD1, 0x68, 0x0A, 0, 0 };
    std::memcpy(code.data() + 0x180 + 39, texture.data(), texture.size());
    for (bool oldGeneration : { true, false }) {
        const std::array<std::uint8_t, 8> map{ 0x42, 0x8B, 0x94, 0x91, static_cast<std::uint8_t>(oldGeneration ? 0xC4 : 0xF4), 0x0D, 0, 0 };
        const auto mapOffset = oldGeneration ? 8U : 12U;
        const auto multiplyOffset = oldGeneration ? 32U : 35U;
        std::memcpy(code.data() + 0x100 + mapOffset, map.data(), map.size());
        std::memcpy(code.data() + 0x180 + multiplyOffset, multiply.data(), multiply.size());
        if (!PPM::Screenshot::LayoutMatches(saver, manager, renderer, oldGeneration, begin, end)) return 1;
        const auto rejectsMutation = [&](std::size_t position) {
            code[position] ^= 1;
            const bool rejected = !PPM::Screenshot::LayoutMatches(saver, manager, renderer, oldGeneration, begin, end);
            code[position] ^= 1;
            return rejected;
        };
        if (!rejectsMutation(0x91) || !rejectsMutation(0x100 + 0x1B) ||
            !rejectsMutation(0x100 + mapOffset + 4) || !rejectsMutation(0x180 + multiplyOffset) ||
            !rejectsMutation(0x180 + 42)) return 2;
        if (PPM::Screenshot::LayoutMatches(saver, manager, renderer, oldGeneration, begin, renderer + 49)) return 3;
    }
    branch(renderer + 80, 0xE9, saver);
    if (!PPM::Screenshot::BranchMatches(renderer + 80, 0xE9, saver, begin, end)) return 4;
    if (PPM::Screenshot::BranchMatches(renderer + 80, 0xE8, saver, begin, end)) return 5;
    if (PPM::Screenshot::BranchMatches(renderer + 80, 0xE9, manager, begin, end)) return 6;
    return 0;
}
