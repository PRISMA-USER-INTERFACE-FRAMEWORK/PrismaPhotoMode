#include "ScreenshotBlend.h"
#include <array>
#include <cstdint>

int main()
{
    std::array<std::uint8_t, 24> base{ 10, 20, 30, 255, 10, 20, 30, 255, 99, 99, 99, 99,
                                       10, 20, 30, 255, 10, 20, 30, 255, 99, 99, 99, 99 };
    const std::array<std::uint8_t, 24> overlay{ 200, 100, 50, 0, 200, 100, 50, 255, 77, 77, 77, 77,
                                               110, 120, 130, 128, 110, 120, 130, 128, 77, 77, 77, 77 };
    PPM::Screenshot::BlendRGBA(base.data(), 12, overlay.data(), 12, 2, 2);
    const std::array<std::uint8_t, 24> expected{ 10, 20, 30, 255, 200, 100, 50, 255, 99, 99, 99, 99,
                                               60, 70, 80, 255, 60, 70, 80, 255, 99, 99, 99, 99 };
    return base == expected ? 0 : 1;
}
