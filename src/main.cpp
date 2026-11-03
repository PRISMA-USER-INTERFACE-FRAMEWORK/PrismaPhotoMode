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
#include <RE/P/PauseMenu.h>
#include <RE/P/PipboyMenu.h>
#include <RE/U/UI.h>
#include <RE/U/UIMessageQueue.h>

#include "PrismaUI_F4_API.h"

#include "PhotoMode.h"
#include "PhotoModeLists.h"
#include "RuntimeSupport.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cmath>
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
    constexpr char kMcmIniPath[] = "Data\\MCM\\Settings\\PrismaPhotoMode.ini";
    constexpr char kCommandListener[] = "pm_command";
    constexpr char kStateCallback[] = "pmSetState";
    constexpr char kListsCallback[] = "pmSetLists";
    constexpr char kOverlaysCallback[] = "pmSetOverlays";
    constexpr char kFiltersCallback[] = "pmSetFilters";
    constexpr char kCharactersCallback[] = "pmSetCharacters";
    constexpr char kPickCallback[] = "pmSetPick";
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
    constexpr std::array kFocusPadButtons{
        static_cast<RE::BS_BUTTON_CODE>(0), RE::BS_BUTTON_CODE::kDPAD_Up, RE::BS_BUTTON_CODE::kDPAD_Down,
        RE::BS_BUTTON_CODE::kDPAD_Left, RE::BS_BUTTON_CODE::kDPAD_Right, RE::BS_BUTTON_CODE::kStart,
        RE::BS_BUTTON_CODE::kBack, RE::BS_BUTTON_CODE::kLStick, RE::BS_BUTTON_CODE::kRStick,
        RE::BS_BUTTON_CODE::kLShoulder, RE::BS_BUTTON_CODE::kRShoulder, RE::BS_BUTTON_CODE::kAButton,
        RE::BS_BUTTON_CODE::kBButton, RE::BS_BUTTON_CODE::kXButton, RE::BS_BUTTON_CODE::kYButton,
        RE::BS_BUTTON_CODE::kLTrigger, RE::BS_BUTTON_CODE::kRTrigger
    };
    constexpr int kDefaultPanelScale = 100;
    constexpr int kMinPanelScale = 50;
    constexpr int kMaxPanelScale = 200;
    constexpr float kMinDofDistance = 50.0F;
    constexpr float kMaxDofDistance = 5000.0F;
    constexpr float kMaxDofRange = 5000.0F;
    constexpr float kMaxLighting = 3.0F;
    constexpr float kMaxRoll = 90.0F;
    constexpr std::size_t kTrampolineSize = 64;

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
    bool g_pauseHookReady = false;
    std::atomic_bool g_pauseEntryPending{ false };
    std::atomic_uint64_t g_pauseRequestGeneration{ 0 };

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
        const auto toggleKey = GetPrivateProfileIntA("General", "iToggleKey", static_cast<int>(g_settings.toggleKey), kMcmIniPath);
        const auto focusKey = GetPrivateProfileIntA("General", "iFocusKey", static_cast<int>(g_settings.focusKey), kMcmIniPath);
        if (toggleKey > 0 && toggleKey < 256) g_settings.toggleKey = toggleKey;
        if (focusKey > 0 && focusKey < 256) g_settings.focusKey = focusKey;
        const auto focusPadIndex = GetPrivateProfileIntA("General", "iFocusPadButton", -1, kMcmIniPath);
        if (focusPadIndex < kFocusPadButtons.size()) g_settings.focusPadButton = static_cast<std::uint32_t>(kFocusPadButtons[focusPadIndex]);
        g_settings.panelScale = std::clamp(
            static_cast<int>(GetPrivateProfileIntA("General", "iPanelScale",
                GetPrivateProfileIntA("General", "PanelScale", kDefaultPanelScale, kIniPath), kMcmIniPath)),
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
            R"({{"active":{},"focused":{},"fov":{:.1f},"speed":{:.2f},"frozen":{},"timeMult":{:.2f},"hour":{:.2f},"timescale":{:.1f},"timeReady":{},"panelScale":{},"lightingReady":{},"brightness":{:.2f},"saturation":{:.2f},"contrast":{:.2f},"roll":{:.1f},"rollReady":{}}})",
            active ? "true" : "false",
            focused ? "true" : "false",
            PPM::PhotoMode::CurrentFOV(),
            PPM::PhotoMode::CurrentTranslateSpeed(),
            PPM::PhotoMode::Frozen() ? "true" : "false",
            PPM::PhotoMode::TimeMultiplier(),
            PPM::PhotoMode::GameHour(),
            PPM::PhotoMode::TimescaleMultiplier(),
            PPM::RuntimeSupport::TimeControlsAvailable() ? "true" : "false",
            g_settings.panelScale,
            PPM::PhotoMode::LightingActive() ? "true" : "false",
            PPM::PhotoMode::Brightness(),
            PPM::PhotoMode::Saturation(),
            PPM::PhotoMode::Contrast(),
            PPM::PhotoMode::ViewRoll(),
            PPM::RuntimeSupport::ViewRollAvailable() ? "true" : "false");
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

    void PushFilters()
    {
        if (!ViewReady() || !PPM::RuntimeSupport::FiltersAvailable()) return;
        const auto json = PPM::PhotoModeLists::BuildFilters();
        g_api->InteropCall(g_view, kFiltersCallback, json.c_str());
    }

    void PushCharacters()
    {
        if (!ViewReady() || !PPM::RuntimeSupport::CharactersAvailable()) return;
        const auto json = PPM::PhotoModeLists::BuildCharacterForms();
        g_api->InteropCall(g_view, kCharactersCallback, json.c_str());
    }

    void PushPick()
    {
        if (!ViewReady() || !PPM::RuntimeSupport::CharactersAvailable()) return;
        const auto json = PPM::PhotoModeLists::BuildPick(PPM::PhotoMode::PickedName());
        g_api->InteropCall(g_view, kPickCallback, json.c_str());
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
        PushFilters();
        PushCharacters();
    }

    void CancelPauseEntry()
    {
        g_pauseEntryPending = false;
        ++g_pauseRequestGeneration;
    }

    bool AddPauseEntry()
    {
        auto* ui = RE::UI::GetSingleton();
        if (!g_pauseHookReady || !ViewReady() || !ui) return false;
        auto menu = ui->GetMenu<RE::PauseMenu>();
        if (!menu || !menu->uiMovie) return false;
        auto& movie = menu->uiMovie;
        RE::Scaleform::GFx::Value list, entries;
        if (!movie->GetVariable(&list, "root1.Menu_mc.MainPanel_mc.List_mc") || !list.IsObject() ||
            !list.GetMember("entryList", &entries) || !entries.IsArray()) return false;
        RE::Scaleform::GFx::Value index;
        bool found = false;
        for (std::uint32_t i = 0; i < entries.GetArraySize(); ++i) {
            RE::Scaleform::GFx::Value item, text, marker, itemIndex;
            if (!entries.GetElement(i, &item) || !item.IsObject()) continue;
            if (item.GetMember("prismaPhotoMode", &marker) && marker.IsBoolean() && marker.GetBoolean()) {
                if (!item.SetMember("disabled", ui->GetMenuOpen<RE::PipboyMenu>())) return false;
                list.Invoke("InvalidateData");
                return true;
            }
            if (item.GetMember("text", &text) && text.IsString() && text.GetString() &&
                std::string_view(text.GetString()) == "$INSTALLED CONTENT" && item.GetMember("index", &itemIndex) &&
                (itemIndex.IsNumber() || itemIndex.IsInt() || itemIndex.IsUInt())) {
                index = itemIndex;
                found = true;
            }
        }
        if (!found) return false;
        RE::Scaleform::GFx::Value entry;
        movie->CreateObject(&entry);
        if (!entry.IsObject() || !entry.SetMember("text", "PHOTO MODE") ||
            !entry.SetMember("disabled", ui->GetMenuOpen<RE::PipboyMenu>()) ||
            !entry.SetMember("index", index) || !entry.SetMember("prismaPhotoMode", true) ||
            !entries.PushBack(entry)) return false;
        list.Invoke("InvalidateData");
        return true;
    }

    using PauseMenuCall = void (*)(RE::PauseMenu*, const RE::Scaleform::GFx::FunctionHandler::Params&);
    PauseMenuCall g_originalPauseMenuCall = nullptr;

    void OnPauseMenuCall(RE::PauseMenu* a_menu, const RE::Scaleform::GFx::FunctionHandler::Params& a_params)
    {
        if (reinterpret_cast<std::uintptr_t>(a_params.userData) == 16 && a_menu && a_menu->uiMovie && ViewReady()) {
            RE::Scaleform::GFx::Value entry, marker, disabled;
            auto* queue = RE::UIMessageQueue::GetSingleton();
            if (queue && F4SE::GetTaskInterface() &&
                a_menu->uiMovie->GetVariable(&entry, "root1.Menu_mc.MainPanel_mc.List_mc.selectedEntry") && entry.IsObject() &&
                entry.GetMember("prismaPhotoMode", &marker) && marker.IsBoolean() && marker.GetBoolean() &&
                entry.GetMember("disabled", &disabled) && disabled.IsBoolean() && !disabled.GetBoolean()) {
                g_pauseEntryPending = true;
                queue->AddMessage("PauseMenu", RE::UI_MESSAGE_TYPE::kHide);
                return;
            }
        }
        if (g_originalPauseMenuCall) g_originalPauseMenuCall(a_menu, a_params);
    }

    void InstallPauseHook()
    {
        REL::Relocation<std::uintptr_t> pauseTable{
            RE::VTABLE::PauseMenu[0] // E:/RE-work/PrismaPhotoMode-RE-Work/pause-menu.md
        };
        MEMORY_BASIC_INFORMATION info{};
        if (!pauseTable.address() || !VirtualQuery(reinterpret_cast<const void*>(pauseTable.address()), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return;
        const auto original = reinterpret_cast<const std::uintptr_t*>(pauseTable.address())[1];
        if (!original || !VirtualQuery(reinterpret_cast<const void*>(original), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || !(info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return;
        g_originalPauseMenuCall = reinterpret_cast<PauseMenuCall>(pauseTable.write_vfunc(1, OnPauseMenuCall));
        g_pauseHookReady = g_originalPauseMenuCall != nullptr;
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
        return result.ec == std::errc{} && result.ptr == last && std::isfinite(a_value);
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
        } else if (name == "roll") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetViewRoll(std::clamp(number, -kMaxRoll, kMaxRoll));
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
        } else if (name == "dofstrength") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetDepthOfFieldStrength(std::clamp(number, 0.0F, 1.0F));
        } else if (name == "dofrange") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetDepthOfFieldRange(std::clamp(number, 0.0F, kMaxDofRange));
        } else if (name == "autofocus") {
            PPM::PhotoMode::SetAutoFocus(value == "1");
        } else if (name == "brightness") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetBrightness(std::clamp(number, 0.0F, kMaxLighting));
        } else if (name == "saturation") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetSaturation(std::clamp(number, 0.0F, kMaxLighting));
        } else if (name == "contrast") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetContrast(std::clamp(number, 0.0F, kMaxLighting));
        } else if (name == "lightreset") {
            PPM::PhotoMode::ResetLighting();
            PushState();
        } else if (name == "filter") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::SetFilter(id);
        } else if (name == "filterstrength") {
            if (ParseFloat(value, number)) PPM::PhotoMode::SetFilterStrength(std::clamp(number, 0.0F, 1.0F));
        } else if (name == "pick") {
            PushPick();
        } else if (name == "pickfreeze") {
            PPM::PhotoMode::SetPickedFrozen(value == "1");
        } else if (name == "face") {
            PPM::PhotoMode::FacePicked();
        } else if (name == "idle") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::PlayIdleOnPick(id);
        } else if (name == "playeridle") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::PlayIdleOnPlayer(id);
        } else if (name == "expression") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::SetExpressionOnPick(id);
        } else if (name == "playerexpression") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::SetExpressionOnPlayer(id);
        } else if (name == "art") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::ApplyArtToPick(id);
        } else if (name == "shader") {
            std::uint32_t id = 0;
            ParseUnsigned(value, id);
            PPM::PhotoMode::ApplyShaderToPick(id);
        } else if (name == "showplayer") {
            PPM::PhotoMode::SetPlayerVisible(value == "1");
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
        PushFilters();
        PushCharacters();
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
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            if (a_event.menuName == "PauseMenu") {
                if (a_event.opening) {
                    CancelPauseEntry();
                    AddPauseEntry();
                } else {
                    LoadSettings();
                    if (g_pauseEntryPending.exchange(false)) {
                        const auto generation = g_pauseRequestGeneration.load();
                        if (auto* tasks = F4SE::GetTaskInterface()) {
                            tasks->AddTask([generation] {
                                if (generation == g_pauseRequestGeneration.load()) EnterMode();
                            });
                        }
                    }
                }
            }
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
            PPM::RuntimeSupport::ValidateFilterMappings(g_runtimeFamily);
            PPM::RuntimeSupport::ValidateLightingMappings(g_runtimeFamily);
            PPM::RuntimeSupport::ValidateViewRollMappings(g_runtimeFamily);
            PPM::PhotoMode::InstallViewRoll();
            PPM::RuntimeSupport::ValidateExpressionMappings(g_runtimeFamily);
            PPM::RuntimeSupport::ValidateCharacterMappings(g_runtimeFamily);
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
            InstallPauseHook();
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
            CancelPauseEntry();
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
    v.UsesAddressLibraryNG(true);
    v.UsesAddressLibrary(true);
    v.UsesSigScanning(false);
    v.IsLayoutDependent(true);
    v.IsLayoutDependentNG(true);
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
    if (g_runtimeFamily == PPM::RuntimeSupport::Family::Unsupported) SetupLogging();
    g_runtimeVersion = f4se->RuntimeVersion();
    g_runtimeFamily = PPM::RuntimeSupport::Classify(g_runtimeVersion);
    if (g_runtimeFamily == PPM::RuntimeSupport::Family::Unsupported || f4se->IsEditor()) {
        spdlog::error("PrismaPhotoMode: unsupported Fallout runtime {} or editor", g_runtimeVersion.string());
        return false;
    }
    F4SE::Init(f4se, F4SE::InitInfo{ .log = false, .trampoline = true, .trampolineSize = kTrampolineSize });
    auto* messaging = F4SE::GetMessagingInterface();
    if (!messaging) return false;
    messaging->RegisterListener(HandleMessage);
    spdlog::info("PrismaPhotoMode loaded for {} {}", PPM::RuntimeSupport::Name(g_runtimeFamily), g_runtimeVersion.string());
    return true;
}
