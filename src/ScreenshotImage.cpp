#include "ScreenshotImage.h"
#include "ScreenshotBlend.h"

namespace PPM::Screenshot
{
    namespace
    {
        bool ToRGBA(const DirectX::Image& input, DirectX::ScratchImage& result)
        {
            auto encoded = input;
            encoded.format = DirectX::MakeLinear(input.format);
            if (encoded.format == DXGI_FORMAT_R8G8B8A8_UNORM) return SUCCEEDED(result.InitializeFromImage(encoded));
            return SUCCEEDED(DirectX::Convert(encoded, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT,
                                              DirectX::TEX_THRESHOLD_DEFAULT, result));
        }
    }

    bool CaptureFramed(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Resource* texture, const std::filesystem::path& frame, DirectX::ScratchImage& result)
    {
        if (!device || !context || !texture) return false;
        DirectX::ScratchImage capture;
        if (FAILED(DirectX::CaptureTexture(device, context, texture, capture))) return false;
        const auto* image = capture.GetImage(0, 0, 0);
        return image && Composite(*image, frame, result);
    }

    bool SavePNG(const DirectX::ScratchImage& image, const std::filesystem::path& path)
    {
        const auto* pixels = image.GetImage(0, 0, 0);
        return pixels && SUCCEEDED(DirectX::SaveToWICFile(*pixels, DirectX::WIC_FLAGS_FORCE_SRGB,
                                  DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), path.c_str()));
    }

    bool Composite(const DirectX::Image& captured, const std::filesystem::path& frame, DirectX::ScratchImage& result)
    {
        if (!captured.width || !captured.height || !ToRGBA(captured, result)) return false;
        DirectX::ScratchImage loaded;
        if (FAILED(DirectX::LoadFromWICFile(frame.c_str(), DirectX::WIC_FLAGS_FORCE_RGB | DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, loaded))) return false;
        const auto* image = loaded.GetImage(0, 0, 0);
        if (!image) return false;
        DirectX::ScratchImage rgba;
        if (!ToRGBA(*image, rgba)) return false;
        const auto* overlay = rgba.GetImage(0, 0, 0);
        if (!overlay) return false;
        DirectX::ScratchImage resized;
        if (overlay->width != captured.width || overlay->height != captured.height) {
            if (FAILED(DirectX::Resize(*overlay, captured.width, captured.height,
                                       DirectX::TEX_FILTER_LINEAR | DirectX::TEX_FILTER_FORCE_WIC, resized))) return false;
            overlay = resized.GetImage(0, 0, 0);
        }
        const auto* background = result.GetImage(0, 0, 0);
        if (!background || !overlay) return false;
        BlendRGBA(background->pixels, background->rowPitch, overlay->pixels, overlay->rowPitch,
                  background->width, background->height);
        return true;
    }
}
