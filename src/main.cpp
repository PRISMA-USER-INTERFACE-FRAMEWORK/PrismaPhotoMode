// CommonLibF4 headers MUST be parsed before PrismaUI_F4_API.h (its raw <Windows.h> macros collide with CommonLib KERNEL32 declarations).
#include <F4SE/F4SE.h>
#include <RE/B/BSInputEventUser.h>
#include <RE/B/BS_BUTTON_CODE.h>
#include <RE/B/ButtonEvent.h>
#include <RE/I/INPUT_DEVICE.h>
#include <RE/I/INPUT_EVENT_TYPE.h>
#include <RE/I/InputEvent.h>
#include <RE/M/MenuControls.h>
#include <RE/M/MenuOpenCloseEvent.h>
#include <RE/U/UI.h>

#include "PrismaUI_F4_API.h"

#include "PhotoMode.h"
#include "PhotoModeLists.h"
#include "RuntimeSupport.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#define DLLEXPORT __declspec(dllexport)

namespace
{
    constexpr char kPluginName[] = "PrismaPhotoMode";
    constexpr char kViewPath[] = "PrismaPhotoMode/index.html";
    constexpr char kIniPath[] = "Data\\F4SE\\Plugins\\PrismaPhotoMode.ini";
    constexpr char kCommandListener[] = "pm_command";
    constexpr char kStateCallback[] = "pmSetState";
    constexpr char kListsCallback[] = "pmSetLists";
    constexpr char kOverlaysCallback[] = "pmSetOverlays";
    constexpr float kMinFOV = 5.0F;
    constexpr float kMaxFOV = 150.0F;
    constexpr float kMinSpeed = 0.1F;
    constexpr float kMaxSpeed = 50.0F;
    constexpr float kMinTimeMult = 0.01F;
    constexpr float kMaxTimeMult = 2.0F;
    constexpr float kMaxHour = 23.99F;
    constexpr float kMaxTimescale = 1000.0F;
    constexpr std::uint32_t kDefaultToggleKey = 0x78;
    constexpr std::uint32_t kDefaultFocusKey = 0x09;
    constexpr std::uint32_t kDefaultFocusPadButton = 0x10020;
    constexpr int kDefaultPanelScale = 100;
    constexpr int kMinPanelScale = 50;
    constexpr int kMaxPanelScale = 200;
    constexpr float kMinDofDistance = 50.0F;
    constexpr float kMaxDofDistance = 5000.0F;

    struct Settings
    {
        std::uint32_t toggleKey{ kDefaultToggleKey };
        std::uint32_t focusKey{ kDefaultFocusKey };
        std::uint32_t focusPadButton{ kDefaultFocusPadButton };
        int panelScale{ kDefaultPanelScale };
    };

    Settings g_settings{};
    PRISMA_UI_API::IVPrismaUI10* g_api = nullptr;
    PrismaView g_view = 0;
    PPM::RuntimeSupport::Family g_runtimeFamily = PPM::RuntimeSupport::Family::Unsupported;
    REL::Version g_runtimeVersion{};
    bool g_runtimeReady = false;
    bool g_domReady = false;

    std::uint32_t ReadKey(const char* a_name, std::uint32_t a_fallback)
    {
        const auto value = GetPrivateProfileIntA("General", a_name, static_cast<int>(a_fallback), kIniPath);
        return value > 0 && value < 256 ? static_cast<std::uint32_t>(value) : a_fallback;
    }

    void LoadSettings()
    {
        g_settings.toggleKey = ReadKey("ToggleKey", kDefaultToggleKey);
        g_settings.focusKey = ReadKey("FocusKey", kDefaultFocusKey);
        g_settings.focusPadButton = GetPrivateProfileIntA("General", "FocusPadButton", static_cast<int>(kDefaultFocusPadButton), kIniPath);
        g_settings.panelScale = std::clamp(
            static_cast<int>(GetPrivateProfileIntA("General", "PanelScale", kDefaultPanelScale, kIniPath)),
            kMinPanelScale,
            kMaxPanelScale);
    }

    bool ViewReady()
    {
        return g_runtimeReady && g_domReady && g_api && g_view && g_api->IsValid(g_view);
    }

    void PushState()
    {
        if (!ViewReady()) return;
        const bool active = PPM::PhotoMode::Active();
        const bool focused = active && g_api->HasFocus(g_view);
        const auto json = std::format(
            R"({{"active":{},"focused":{},"fov":{:.1f},"speed":{:.2f},"frozen":{},"timeMult":{:.2f},"hour":{:.2f},"timescale":{:.1f},"timeReady":{},"panelScale":{}}})",
            active ? "true" : "false",
            focused ? "true" : "false",
            PPM::PhotoMode::CurrentFOV(),
            PPM::PhotoMode::CurrentTranslateSpeed(),
            PPM::PhotoMode::Frozen() ? "true" : "false",
            PPM::PhotoMode::TimeMultiplier(),
            PPM::PhotoMode::GameHour(),
            PPM::PhotoMode::TimescaleMultiplier(),
            PPM::RuntimeSupport::TimeControlsAvailable() ? "true" : "false",
            g_settings.panelScale);
        g_api->InteropCall(g_view, kStateCallback, json.c_str());
    }

    void PushLists()
    {
        if (!ViewReady() || !PPM::RuntimeSupport::TimeControlsAvailable()) return;
        const auto json = PPM::PhotoModeLists::Build();
        g_api->InteropCall(g_view, kListsCallback, json.c_str());
    }

    void PushOverlays()
    {
        if (!ViewReady()) return;
        const auto json = PPM::PhotoModeLists::BuildOverlays();
        g_api->InteropCall(g_view, kOverlaysCallback, json.c_str());
    }

    void SetFocus(bool a_focus)
    {
        if (!ViewReady()) return;
        if (a_focus) {
            if (!g_api->Focus(g_view, false, true)) spdlog::warn("PrismaPhotoMode: panel focus was refused");
        } else if (g_api->HasFocus(g_view)) {
            g_api->Unfocus(g_view);
        }
        PushState();
    }

    void EnterMode()
    {
        if (!ViewReady() || !PPM::PhotoMode::Enter()) return;
        g_api->Show(g_view);
        PushState();
        PushLists();
        PushOverlays();
    }

    void ExitMode()
    {
        if (!PPM::PhotoMode::Active()) return;
        if (ViewReady()) {
            if (g_api->HasFocus(g_view)) g_api->Unfocus(g_view);
            g_api->Hide(g_view);
        }
        PPM::PhotoMode::Exit();
        PushState();
    }

    bool ParseFloat(std::string_view a_text, float& a_value)
    {
        const auto* first = a_text.data();
        const auto* last = first + a_text.size();
        const auto result = std::from_chars(first, last, a_value);
        return result.ec == std::errc{} && result.ptr == last;
    }

    bool ParseUnsigned(std::string_view a_text, std::uint32_t& a_value)
    {
        const auto* first = a_text.data();
        const auto* last = first + a_text.size();
        const auto result = std::from_chars(first, last, a_value);
        return result.ec == std::errc{} && result.ptr == last;
    }

    void HandleCommand(const char* a_argument)
    {
        if (!a_argument) return;
        const std::string_view text{ a_argument };
        const auto separator = text.find('|');
        const auto name = text.substr(0, separator);
        const auto value = separator == std::string_view::npos ? std::string_view{} : text.substr(separator + 1);
        float number = 0.0F;

        if (name == "fov") {
            if (ParseFloat(value, number) && PPM::PhotoMode::Active()) PPM::PhotoMode::SetFOV(std::clamp(number, kMinFOV, kMaxFOV));
        } else if (name == "speed") {
            if (ParseFloat(value, number) && PPM::PhotoMode::Active()) PPM::PhotoMode::SetTranslateSpeed(std::clamp(number, kMinSpeed, kMaxSpeed));
        } else if (name == "focus") {
            SetFocus(value == "1");
        } else if (name == "screenshot") {
            if (PPM::PhotoMode::Active() && !PPM::PhotoMode::QueueScreenshot()) spdlog::warn("PrismaPhotoMode: screenshot request was not queued");
        } else if (name == "reset") {
            PPM::PhotoMode::ResetCamera();
            PushState();
        } else if (name == "freeze") {
            PPM::PhotoMode::SetFrozen(value == "1");
            PushState();
        } else if (name == "timemult") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetTimeMultiplier(std::clamp(number, kMinTimeMult, kMaxTimeMult));
        } else if (name == "hour") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetGameHour(std::clamp(number, 0.0F, kMaxHour));
        } else if (name == "timescale") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetTimescaleMultiplier(std::clamp(number, 1.0F, kMaxTimescale));
        } else if (name == "weather") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::SetWeather(id);
        } else if (name == "dof") {
            PPM::PhotoMode::SetDepthOfField(value == "1");
        } else if (name == "dofdist") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetDepthOfFieldDistance(std::clamp(number, kMinDofDistance, kMaxDofDistance));
        } else if (name == "autofocus") {
            PPM::PhotoMode::SetAutoFocus(value == "1");
        } else if (name == "lists") {
            PushLists();
        } else if (name == "exit") {
            ExitMode();
        } else {
            spdlog::warn("PrismaPhotoMode: unknown or malformed command '{}'", name);
        }
    }

    void OnDomReady(PrismaView a_view)
    {
        if (!g_api || a_view != g_view) return;
        g_api->SetViewRole(a_view, PRISMA_UI_API::ViewRole::kPanel);
        g_api->SetViewOwnsEscape(a_view, true);
        g_api->BindUIEvent(a_view, kCommandListener, HandleCommand);
        if (!PPM::PhotoMode::Active()) g_api->Hide(a_view);
        g_domReady = true;
        PushState();
        PushLists();
        PushOverlays();
    }

    class PhotoModeInput final : public RE::BSInputEventUser
    {
    public:
        bool ShouldHandleEvent(const RE::InputEvent* a_event) override
        {
            return a_event && a_event->eventType.get() == RE::INPUT_EVENT_TYPE::kButton;
        }

        void OnButtonEvent(const RE::ButtonEvent* a_event) override
        {
            if (!a_event || !a_event->QJustPressed()) return;

            if (a_event->device.get() == RE::INPUT_DEVICE::kGamepad) {
                if (g_settings.focusPadButton != 0 && a_event->idCode == g_settings.focusPadButton && PPM::PhotoMode::Active()) {
                    SetFocus(!(ViewReady() && g_api->HasFocus(g_view)));
                }
                return;
            }
            if (a_event->device.get() != RE::INPUT_DEVICE::kKeyboard) return;

            if (a_event->idCode == g_settings.toggleKey) {
                if (PPM::PhotoMode::Active()) {
                    ExitMode();
                } else {
                    EnterMode();
                }
            } else if (a_event->idCode == g_settings.focusKey && PPM::PhotoMode::Active()) {
                SetFocus(!(ViewReady() && g_api->HasFocus(g_view)));
            }
        }
    };

    PhotoModeInput g_input{};

    class PhotoModeMenuWatcher final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent&, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            if (PPM::PhotoMode::Active() && !PPM::PhotoMode::InGameplayContext()) ExitMode();
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    PhotoModeMenuWatcher g_menuWatcher{};

    void SetupLogging()
    {
        try {
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Data/F4SE/Plugins/PrismaPhotoMode.log", true);
            auto log = std::make_shared<spdlog::logger>(kPluginName, std::move(sink));
            log->set_level(spdlog::level::info);
            log->flush_on(spdlog::level::info);
            spdlog::set_default_logger(std::move(log));
            spdlog::set_pattern("[%Y-%m-%d %T.%e] [%l] %v");
        } catch (...) {
        }
    }

    void HandleMessage(F4SE::MessagingInterface::Message* a_message)
    {
        if (!a_message) return;
        switch (a_message->type) {
        case F4SE::MessagingInterface::kGameDataReady: {
            LoadSettings();
            if (!PPM::RuntimeSupport::ValidateMappings(g_runtimeFamily)) {
                spdlog::critical("PrismaPhotoMode: required runtime mappings are unavailable; photo mode disabled");
                return;
            }
            PPM::RuntimeSupport::ValidateTimeMappings(g_runtimeFamily);
            PPM::RuntimeSupport::ValidateDepthOfFieldMappings(g_runtimeFamily);
            g_api = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI10>();
            if (!g_api) {
                spdlog::error("PrismaPhotoMode: PrismaUI_F4 V10 API unavailable; photo mode disabled");
                return;
            }
            g_view = g_api->CreateView(kViewPath, OnDomReady);
            if (!g_view) {
                spdlog::error("PrismaPhotoMode: failed to create view '{}'", kViewPath);
                return;
            }
            RE::MenuControls::GetSingleton()->handlers.emplace_back(&g_input);
            RE::UI::GetSingleton()->RegisterSink<RE::MenuOpenCloseEvent>(&g_menuWatcher);
            g_runtimeReady = true;
            spdlog::info(
                "PrismaPhotoMode: runtime ready family={} version={} toggleKey={} focusKey={}",
                PPM::RuntimeSupport::Name(g_runtimeFamily),
                g_runtimeVersion.string(),
                g_settings.toggleKey,
                g_settings.focusKey);
            break;
        }

        case F4SE::MessagingInterface::kNewGame:
        case F4SE::MessagingInterface::kPreLoadGame:
        case F4SE::MessagingInterface::kPostLoadGame:
            ExitMode();
            break;

        default:
            break;
        }
    }
}

F4SE_PLUGIN_VERSION = []() noexcept {
    F4SE::PluginVersionData v{};
    v.PluginVersion({ 1, 0, 0, 0 });
    v.PluginName(kPluginName);
    v.AuthorName("NomadsReach");
    v.UsesAddressLibrary(true);
    v.UsesSigScanning(false);
    v.IsLayoutDependent(true);
    v.HasNoStructUse(false);
    return v;
}();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info)
{
    SetupLogging();
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = kPluginName;
    info->version = 1;
    if (f4se->IsEditor()) return false;

    g_runtimeVersion = f4se->RuntimeVersion();
    g_runtimeFamily = PPM::RuntimeSupport::Classify(g_runtimeVersion);
    if (g_runtimeFamily == PPM::RuntimeSupport::Family::Unsupported) {
        spdlog::error(
            "PrismaPhotoMode: unsupported Fallout runtime {}. Supported runtimes are OG 1.10.163, NG 1.10.984 and AE 1.11.137 or later",
            g_runtimeVersion.string());
        return false;
    }

    spdlog::info(
        "PrismaPhotoMode: accepted Fallout runtime family={} version={}",
        PPM::RuntimeSupport::Name(g_runtimeFamily),
        g_runtimeVersion.string());
    return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* f4se)
{
    if (g_runtimeFamily == PPM::RuntimeSupport::Family::Unsupported) return false;
    F4SE::Init(f4se, F4SE::InitInfo{ .log = false });
    auto* messaging = F4SE::GetMessagingInterface();
    if (!messaging) return false;
    messaging->RegisterListener(HandleMessage);
    spdlog::info("PrismaPhotoMode loaded for {} {}", PPM::RuntimeSupport::Name(g_runtimeFamily), g_runtimeVersion.string());
    return true;
}
