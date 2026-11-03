#pragma once

#include <DirectXTex.h>
#include <filesystem>

namespace PPM::Screenshot
{
    bool Composite(const DirectX::Image& captured, const std::filesystem::path& frame, DirectX::ScratchImage& result);
}
