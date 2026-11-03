#pragma once

#include <cstddef>
#include <cstdint>

namespace PPM::Screenshot
{
    inline void BlendRGBA(std::uint8_t* destination, std::size_t destinationPitch, const std::uint8_t* overlay,
                          std::size_t overlayPitch, std::size_t width, std::size_t height) noexcept
    {
        for (std::size_t y = 0; y < height; ++y) {
            auto* dst = destination + y * destinationPitch;
            const auto* src = overlay + y * overlayPitch;
            for (std::size_t x = 0; x < width; ++x, dst += 4, src += 4) {
                const unsigned alpha = src[3];
                for (std::size_t channel = 0; channel < 3; ++channel) {
                    dst[channel] = static_cast<std::uint8_t>((src[channel] * alpha + dst[channel] * (255 - alpha) + 127) / 255);
                }
                dst[3] = 255;
            }
        }
    }
}
