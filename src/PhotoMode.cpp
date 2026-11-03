#include "PhotoMode.h"

#include <RE/C/CameraStates.h>
#include <RE/B/BGSSaveLoadManager.h>
#include <RE/C/ControlMap.h>
#include <RE/H/HUDModeType.h>
#include <RE/M/Main.h>
#include <RE/M/MenuControls.h>
#include <RE/P/PlayerCamera.h>
#include <RE/S/SendHUDMessage.h>
#include <RE/S/Setting.h>
#include <RE/U/UserEvents.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>

namespace PPM::PhotoMode
{
    namespace
    {
        constexpr char kTranslateSpeedSetting[] = "fFreeCameraTranslationSpeed:Camera";
        constexpr char kHudMode[] = "DialogueCameraMode";

        struct Snapshot
        {
            bool freeCamera{ false };
            bool freezeTime{ false };
            bool savingAllowed{ true };
            float fov{ 0.0F };
            float translateSpeed{ 0.0F };
        };

        Snapshot g_snapshot{};
        bool g_active{ false };

        bool IsFreeCamera(RE::PlayerCamera* a_camera)
        {
            return a_camera->currentState && a_camera->currentState->id.get() == RE::CameraState::kFree;
        }

        bool GameplayContext()
        {
            constexpr std::array allowed{
                RE::UserEvents::INPUT_CONTEXT_ID::kMainGameplay,
                RE::UserEvents::INPUT_CONTEXT_ID::kSitWait,
                RE::UserEvents::INPUT_CONTEXT_ID::kTFC
            };
            const auto* controlMap = RE::ControlMap::GetSingleton();
            if (!controlMap || controlMap->contextPriorityStack.size() == 0) return false;
            return std::ranges::contains(allowed, controlMap->contextPriorityStack.back());
        }
    }

    bool Active() noexcept
    {
        return g_active;
    }

    bool InGameplayContext()
    {
        return GameplayContext();
    }

    bool Enter()
    {
        auto* camera = RE::PlayerCamera::GetSingleton();
        auto* main = RE::Main::GetSingleton();
        auto* saves = RE::BGSSaveLoadManager::GetSingleton();
        if (g_active || !camera || !main || !saves || !main->gameActive || !GameplayContext()) return false;

        g_snapshot.freeCamera = IsFreeCamera(camera);
        g_snapshot.freezeTime = main->freezeTime;
        g_snapshot.savingAllowed = saves->savingAllowed;
        g_snapshot.fov = camera->worldFOV;
        g_snapshot.translateSpeed = CurrentTranslateSpeed();

        if (!g_snapshot.freeCamera) {
            camera->ToggleFreeCameraMode(false);
            if (!IsFreeCamera(camera)) {
                spdlog::warn("PrismaPhotoMode: free camera did not engage; photo mode not entered");
                return false;
            }
        }
        main->freezeTime = true;
        saves->savingAllowed = false;
        RE::SendHUDMessage::PushHUDMode(RE::HUDModeType(kHudMode));
        g_active = true;
        spdlog::info("PrismaPhotoMode: entered freeCamera={} fov={:.1f}", g_snapshot.freeCamera, g_snapshot.fov);
        return true;
    }

    void Exit()
    {
        if (!g_active) return;

        auto* camera = RE::PlayerCamera::GetSingleton();
        auto* main = RE::Main::GetSingleton();
        auto* saves = RE::BGSSaveLoadManager::GetSingleton();
        if (camera && !g_snapshot.freeCamera && IsFreeCamera(camera)) camera->ToggleFreeCameraMode(false);
        if (main) main->freezeTime = g_snapshot.freezeTime;
        if (saves) saves->savingAllowed = g_snapshot.savingAllowed;
        SetFOV(g_snapshot.fov);
        SetTranslateSpeed(g_snapshot.translateSpeed);
        RE::SendHUDMessage::PopHUDMode(RE::HUDModeType(kHudMode));
        g_active = false;
        spdlog::info("PrismaPhotoMode: exited");
    }

    void SetFOV(float a_fov)
    {
        if (auto* main = RE::Main::GetSingleton()) main->SetCameraFOV(a_fov);
        if (auto* camera = RE::PlayerCamera::GetSingleton()) camera->worldFOV = a_fov;
    }

    void SetTranslateSpeed(float a_speed)
    {
        if (auto* setting = RE::GetINISetting(kTranslateSpeedSetting)) setting->SetFloat(a_speed);
    }

    void ResetCamera()
    {
        if (!g_active) return;
        SetFOV(g_snapshot.fov);
        SetTranslateSpeed(g_snapshot.translateSpeed);
    }

    float CurrentFOV() noexcept
    {
        const auto* camera = RE::PlayerCamera::GetSingleton();
        return camera ? camera->worldFOV : 0.0F;
    }

    float CurrentTranslateSpeed() noexcept
    {
        const auto* setting = RE::GetINISetting(kTranslateSpeedSetting);
        return setting ? setting->GetFloat() : 0.0F;
    }

    bool QueueScreenshot()
    {
        const auto* controls = RE::MenuControls::GetSingleton();
        return controls && controls->QueueScreenshot();
    }
}
