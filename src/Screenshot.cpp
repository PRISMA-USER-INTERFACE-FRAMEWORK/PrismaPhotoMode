#include "Screenshot.h"
#include "ScreenshotHook.h"

#include <RE/B/BSGraphics.h>
#include <RE/M/MenuControls.h>
#include <REL/Trampoline.h>
#include <REL/VariantID.h>
#include <REX/FModule.h>
#include <spdlog/spdlog.h>

#include "ScreenshotImage.h"

#include <d3d11.h>
#include <DirectXTex.h>
#include <Windows.h>
#include <ShlObj.h>
#include <KnownFolders.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace PPM::Screenshot
{
    namespace
    {
        constexpr REL::VariantID kSwap{ 1075087, 2228913, 2228913 };
        constexpr REL::VariantID kSave{ 919230, 2229158, 2229158 };
        constexpr REL::VariantID kTargetWriter{ 1216468, 2277214, 2277214 };
        constexpr REL::VariantID kRendererWriter{ 1431452, 2277002, 2277002 };
        constexpr std::size_t kOGCallOffset = 0x2E3;
        constexpr std::size_t kNGCallOffset = 0x2C6;
        constexpr std::size_t kOGTargetMapOffset = 0xDC4;
        constexpr std::size_t kNGTargetMapOffset = 0xDF4;
        constexpr std::size_t kLogicalTargetCount = 100;
        constexpr std::size_t kPhysicalTargetCount = 101;
        using SaveFunction = void (*)(const char*, RE::BSGraphics::TextureFileFormat, std::uint32_t);

        SaveFunction g_original = nullptr;
        RuntimeSupport::Family g_family = RuntimeSupport::Family::Unsupported;
        std::mutex g_mutex;
        std::optional<std::filesystem::path> g_frame;
        bool g_pending = false;

        std::optional<std::string> Decode(std::string_view value)
        {
            std::string result;
            result.reserve(value.size());
            const auto digit = [](char ch) -> int {
                if (ch >= '0' && ch <= '9') return ch - '0';
                if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
                if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
                return -1;
            };
            for (std::size_t i = 0; i < value.size(); ++i) {
                unsigned char ch = static_cast<unsigned char>(value[i]);
                if (ch == '%') {
                    if (i + 2 >= value.size()) return std::nullopt;
                    const int high = digit(value[i + 1]);
                    const int low = digit(value[i + 2]);
                    if (high < 0 || low < 0) return std::nullopt;
                    ch = static_cast<unsigned char>((high << 4) | low);
                    i += 2;
                }
                if (ch < 0x20 || ch == '/' || ch == '\\' || ch == ':' || ch == '|' || ch == 0x7F) return std::nullopt;
                result.push_back(static_cast<char>(ch));
            }
            if (result.empty() || result.back() == ' ' || result.back() == '.') return std::nullopt;
            return result;
        }

        std::optional<std::filesystem::path> FramePath(std::string_view value)
        {
            const auto separator = value.find('|');
            if (separator == std::string_view::npos || value.find('|', separator + 1) != std::string_view::npos) return std::nullopt;
            const auto category = Decode(value.substr(0, separator));
            const auto name = Decode(value.substr(separator + 1));
            if (!category || !name) return std::nullopt;
            auto path = std::filesystem::path("Data/PrismaUI_F4/views/PrismaPhotoMode/overlays/frames") /
                        std::filesystem::u8path(*category) / std::filesystem::u8path(*name);
            auto extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::error_code error;
            if (extension != ".png" || !std::filesystem::is_regular_file(path, error) || error) return std::nullopt;
            return path;
        }

        bool Capture(const std::filesystem::path& frame, std::uint32_t target)
        {
            if (target >= kLogicalTargetCount) return false;
            auto* renderer = RE::BSGraphics::GetRendererData();
            auto* manager = RE::BSGraphics::RenderTargetManager::GetSingleton();
            if (!renderer || !manager || !renderer->device || !renderer->context) return false;
            const auto offset = g_family == RuntimeSupport::Family::OG ? kOGTargetMapOffset : kNGTargetMapOffset;
            std::uint32_t physical = 0;
            std::memcpy(&physical, reinterpret_cast<const std::byte*>(manager) + offset + target * sizeof(physical), sizeof(physical));
            if (physical >= kPhysicalTargetCount || !renderer->renderTargets[physical].texture) return false;

            const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (FAILED(com) && com != RPC_E_CHANGED_MODE) return false;
            struct ComScope {
                bool initialized;
                ~ComScope() { if (initialized) CoUninitialize(); }
            } scope{ SUCCEEDED(com) };
            bool saved = false;
            do {
                DirectX::ScratchImage composed;
                if (!CaptureFramed(reinterpret_cast<ID3D11Device*>(renderer->device),
                        reinterpret_cast<ID3D11DeviceContext*>(renderer->context),
                        reinterpret_cast<ID3D11Resource*>(renderer->renderTargets[physical].texture), frame, composed)) break;
                PWSTR documents = nullptr;
                if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &documents))) break;
                const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> documentsOwner(documents, CoTaskMemFree);
                const auto directory = std::filesystem::path(documentsOwner.get()) / L"My Games/Fallout4/Photos";
                std::error_code error;
                std::filesystem::create_directories(directory, error);
                if (error) break;
                FILETIME now{};
                GetSystemTimeAsFileTime(&now);
                const auto timestamp = (static_cast<std::uint64_t>(now.dwHighDateTime) << 32) | now.dwLowDateTime;
                const auto path = directory / std::format(L"Photo_{}.png", timestamp);
                const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (file == INVALID_HANDLE_VALUE) break;
                CloseHandle(file);
                saved = SavePNG(composed, path);
                if (!saved) std::filesystem::remove(path, error);
                else spdlog::info("PrismaPhotoMode: saved framed screenshot {}", path.string());
            } while (false);
            return saved;
        }

        void SaveThunk(const char* path, RE::BSGraphics::TextureFileFormat format, std::uint32_t target)
        {
            std::optional<std::filesystem::path> frame;
            {
                std::lock_guard lock(g_mutex);
                if (g_pending) {
                    frame = std::exchange(g_frame, std::nullopt);
                    g_pending = false;
                }
            }
            if (frame) {
                try {
                    if (Capture(*frame, target)) return;
                } catch (const std::exception& error) {
                    spdlog::error("PrismaPhotoMode: framed screenshot failed: {}", error.what());
                }
                spdlog::warn("PrismaPhotoMode: framed screenshot failed; using the game screenshot");
            }
            g_original(path, format, target);
        }
    }

    void Install(RuntimeSupport::Family family)
    {
        if (g_original || family == RuntimeSupport::Family::Unsupported) return;
        const auto module = REX::FModule::GetExecutingModule();
        const auto text = module.GetSection(".text");
        const auto start = text.GetAddress();
        const auto end = start + text.GetSize();
        const auto swap = kSwap.address();
        const auto expected = kSave.address();
        const auto offset = family == RuntimeSupport::Family::OG ? kOGCallOffset : kNGCallOffset;
        if (!InText(swap, offset + 5, start, end) || !InText(expected, 1, start, end) ||
            !BranchMatches(swap + offset, 0xE8, expected, start, end) ||
            !LayoutMatches(expected, kTargetWriter.address(), kRendererWriter.address(),
                           family == RuntimeSupport::Family::OG, start, end)) {
            spdlog::warn("PrismaPhotoMode: screenshot hook or render-target layout did not match; framed capture disabled");
            return;
        }
        const auto site = swap + offset;
        g_family = family;
        g_original = reinterpret_cast<SaveFunction>(expected);
        REL::GetTrampoline().write_call<5>(site, &SaveThunk);
        spdlog::info("PrismaPhotoMode: framed screenshot hook installed");
    }

    bool Request(std::string_view frame)
    {
        std::optional<std::filesystem::path> path;
        if (!frame.empty() && g_original) {
            try {
                path = FramePath(frame);
            } catch (const std::exception& error) {
                spdlog::warn("PrismaPhotoMode: invalid screenshot frame: {}", error.what());
            }
        }
        if (!frame.empty() && !path) spdlog::warn("PrismaPhotoMode: framed screenshot unavailable; using the game screenshot");
        const auto* controls = RE::MenuControls::GetSingleton();
        if (!controls) return false;
        if (!g_original) return controls->QueueScreenshot();
        std::lock_guard lock(g_mutex);
        if (g_pending || !controls->QueueScreenshot()) return false;
        g_frame = std::move(path);
        g_pending = true;
        return true;
    }
}
