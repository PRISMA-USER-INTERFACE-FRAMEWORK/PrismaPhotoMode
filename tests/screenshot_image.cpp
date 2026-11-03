#include "ScreenshotImage.h"
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <filesystem>

int main()
{
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized)) return 1;
    wchar_t tempDir[MAX_PATH]{};
    wchar_t framePath[MAX_PATH]{};
    wchar_t resultPath[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, tempDir) || !GetTempFileNameW(tempDir, L"ppm", 0, framePath)) {
        CoUninitialize();
        return 1;
    }
    if (!GetTempFileNameW(tempDir, L"ppm", 0, resultPath)) {
        DeleteFileW(framePath);
        CoUninitialize();
        return 1;
    }
    int status = 1;
    do {
        DirectX::ScratchImage overlay;
        if (FAILED(overlay.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 2, 1, 1, 1))) break;
        auto* pixels = overlay.GetPixels();
        const std::uint8_t overlayPixels[]{ 240, 40, 20, 255, 0, 0, 0, 0 };
        for (int i = 0; i < 8; ++i) pixels[i] = overlayPixels[i];
        if (FAILED(DirectX::SaveToWICFile(*overlay.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE,
                                         DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), framePath))) break;
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                     device.GetAddressOf(), nullptr, context.GetAddressOf()))) break;
        D3D11_TEXTURE2D_DESC descriptor{};
        descriptor.Width = 4;
        descriptor.Height = 2;
        descriptor.MipLevels = 1;
        descriptor.ArraySize = 1;
        descriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        descriptor.SampleDesc.Count = 1;
        descriptor.Usage = D3D11_USAGE_DEFAULT;
        descriptor.BindFlags = D3D11_BIND_RENDER_TARGET;
        std::array<std::uint8_t, 32> base{};
        for (std::size_t i = 0; i < base.size(); i += 4) {
            base[i] = 10; base[i + 1] = 20; base[i + 2] = 30; base[i + 3] = 255;
        }
        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = base.data();
        data.SysMemPitch = 16;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        if (FAILED(device->CreateTexture2D(&descriptor, &data, texture.GetAddressOf()))) break;
        DirectX::ScratchImage capture;
        if (FAILED(DirectX::CaptureTexture(device.Get(), context.Get(), texture.Get(), capture))) break;
        DirectX::ScratchImage result;
        if (!PPM::Screenshot::Composite(*capture.GetImage(0, 0, 0), framePath, result)) break;
        const auto* image = result.GetImage(0, 0, 0);
        if (!image || image->width != 4 || image->height != 2) break;
        if (FAILED(DirectX::SaveToWICFile(*image, DirectX::WIC_FLAGS_FORCE_SRGB,
                                         DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), resultPath))) break;
        DirectX::ScratchImage reloaded;
        if (FAILED(DirectX::LoadFromWICFile(resultPath, DirectX::WIC_FLAGS_FORCE_RGB | DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, reloaded))) break;
        image = reloaded.GetImage(0, 0, 0);
        if (!image || image->format != DXGI_FORMAT_R8G8B8A8_UNORM) break;
        const auto* first = image->pixels;
        const auto* last = image->pixels + 12;
        const auto* secondRow = image->pixels + image->rowPitch;
        if (first[0] != 240 || first[1] != 40 || first[2] != 20 || first[3] != 255) break;
        if (last[0] != 10 || last[1] != 20 || last[2] != 30 || last[3] != 255) break;
        const auto* edge = image->pixels + 4;
        if (edge[0] <= 10 || edge[0] >= 240) break;
        const int redScale = (static_cast<int>(edge[0]) - 10) * 20;
        const int greenScale = (static_cast<int>(edge[1]) - 20) * 230;
        if (redScale - greenScale > 460 || greenScale - redScale > 460) break;
        if (secondRow[0] != first[0] || secondRow[1] != first[1] || secondRow[2] != first[2]) break;
        if (PPM::Screenshot::Composite(*capture.GetImage(0, 0, 0), std::wstring(framePath) + L".missing", result)) break;
        status = 0;
    } while (false);
    DeleteFileW(framePath);
    DeleteFileW(resultPath);
    CoUninitialize();
    return status;
}
