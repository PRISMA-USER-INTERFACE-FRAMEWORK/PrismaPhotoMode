#pragma once

#include <DirectXTex.h>
#include <filesystem>

namespace PPM::Screenshot
{
    bool CaptureFramed(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Resource* texture, const std::filesystem::path& frame, DirectX::ScratchImage& result);
    bool SavePNG(const DirectX::ScratchImage& image, const std::filesystem::path& path);
    bool Composite(const DirectX::Image& captured, const std::filesystem::path& frame, DirectX::ScratchImage& result);
}
