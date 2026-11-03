#include "PhotoMode.h"
#include "RuntimeSupport.h"

#include <RE/A/Actor.h>
#include <RE/A/AIProcess.h>
#include <RE/B/BGSArtObject.h>
#include <RE/B/BGSSaveLoadManager.h>
#include <RE/B/BSPointerHandle.h>
#include <RE/B/BSTimer.h>
#include <RE/C/Calendar.h>
#include <RE/C/CameraStates.h>
#include <RE/C/Console.h>
#include <RE/C/ControlMap.h>
#include <RE/H/HUDModeType.h>
#include <RE/I/ImageSpaceModifierInstanceDOF.h>
#include <RE/I/ImageSpaceModifierInstanceForm.h>
#include <RE/M/Main.h>
#include <RE/M/MenuControls.h>
#include <RE/P/PlayerCamera.h>
#include <RE/S/SendHUDMessage.h>
#include <RE/S/Setting.h>
#include <RE/S/Sky.h>
#include <RE/T/TESDataHandler.h>
#include <RE/T/TESEffectShader.h>
#include <RE/T/TESIdleForm.h>
#include <RE/T/TESImageSpaceModifier.h>
#include <RE/T/TESWeather.h>
#include <RE/U/UserEvents.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

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
            float globalTimeMult{ 1.0F };
            float timescale{ 1.0F };
            float gameHour{ 0.0F };
            RE::TESWeather* weather{ nullptr };
        };

        Snapshot g_snapshot{};
        bool g_active{ false };
        float g_timescaleMult{ 1.0F };
        bool g_weatherForced{ false };

        constexpr float kDefaultDofDistance = 500.0F;
        constexpr float kDofRange = 300.0F;
        constexpr float kDofDuration = 1.0e6F;
        constexpr float kDofRetriggerDistance = 1.0F;

        RE::ImageSpaceModifierInstanceDOF* g_dof{ nullptr };
        float g_dofAppliedDistance{ 0.0F };
        float g_dofDistance{ kDefaultDofDistance };
        bool g_dofOn{ false };
        bool g_autoFocus{ false };

        std::optional<float> PickedDistance()
        {
            if (!RuntimeSupport::DepthOfFieldAvailable()) return std::nullopt;
            const auto ref = RE::Console::GetCurrentPickREFR().get();
            const auto* camera = RE::PlayerCamera::GetSingleton();
            RE::NiPoint3 cameraPosition{};
            if (!ref || !camera || !camera->GetCameraPosition(cameraPosition, true)) return std::nullopt;
            return (ref->GetPosition() - cameraPosition).Length();
        }

        void StopDepthOfField()
        {
            if (!g_dof) return;
            g_dof->Stop();
            g_dof = nullptr;
        }

        void ApplyDepthOfField()
        {
            if (!g_active || !g_dofOn || !RuntimeSupport::DepthOfFieldAvailable()) {
                StopDepthOfField();
                return;
            }
            const auto distance = g_autoFocus ? PickedDistance().value_or(g_dofDistance) : g_dofDistance;
            if (g_dof && std::abs(distance - g_dofAppliedDistance) < kDofRetriggerDistance) return;
            StopDepthOfField();
            g_dof = RE::ImageSpaceModifierInstanceDOF::Trigger(
                distance,
                kDofRange,
                0.0F,
                0.0F,
                RE::ImageSpaceModifierInstanceDOF::DepthOfFieldMode::kFrontBack,
                1.0F,
                kDofDuration);
            g_dofAppliedDistance = distance;
        }

        void ResetDepthOfField()
        {
            StopDepthOfField();
            g_dofOn = false;
            g_autoFocus = false;
            g_dofDistance = kDefaultDofDistance;
        }

        constexpr float kDefaultFilterStrength = 1.0F;

        RE::TESImageSpaceModifier* g_filter{ nullptr };
        float g_filterStrength{ kDefaultFilterStrength };

        template <class T>
        T* FindForm(std::uint32_t a_formID)
        {
            if (!a_formID) return nullptr;
            auto* handler = RE::TESDataHandler::GetSingleton();
            if (!handler) return nullptr;
            for (auto* form : handler->GetFormArray<T>()) {
                if (form && form->GetFormID() == a_formID) return form;
            }
            return nullptr;
        }

        RE::NiPointer<RE::TESObjectREFR> PickedRef()
        {
            return RE::Console::GetCurrentPickREFR().get();
        }

        RE::ObjectRefHandle g_frozen{};

        void StopFilter()
        {
            if (!g_filter) return;
            RE::ImageSpaceModifierInstanceForm::Stop(g_filter);
            g_filter = nullptr;
        }

        void ResetFilter()
        {
            StopFilter();
            g_filterStrength = kDefaultFilterStrength;
        }

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

        RE::TESWeather* FindWeather(std::uint32_t a_formID)
        {
            if (!RuntimeSupport::TimeControlsAvailable() || !a_formID) return nullptr;
            auto* handler = RE::TESDataHandler::GetSingleton();
            if (!handler) return nullptr;
            for (auto* weather : handler->GetFormArray<RE::TESWeather>()) {
                if (weather && weather->GetFormID() == a_formID) return weather;
            }
            return nullptr;
        }

        void ReleaseWeather()
        {
            if (!g_weatherForced) return;
            if (auto* sky = RE::Sky::GetSingleton()) {
                sky->ReleaseWeatherOverride();
                sky->ResetWeather();
                if (g_snapshot.weather) sky->ForceWeather(g_snapshot.weather, false);
            }
            g_weatherForced = false;
        }

        void RestoreTime()
        {
            g_timescaleMult = 1.0F;
            if (!RuntimeSupport::TimeControlsAvailable()) return;
            if (auto* timer = RE::BSTimer::GetSingleton()) timer->SetGlobalTimeMultiplier(g_snapshot.globalTimeMult, true);
            if (auto* calendar = RE::Calendar::GetSingleton()) {
                if (calendar->timeScale) calendar->timeScale->value = g_snapshot.timescale;
                if (calendar->gameHour) calendar->gameHour->value = g_snapshot.gameHour;
            }
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
        if (RuntimeSupport::TimeControlsAvailable()) {
            g_snapshot.globalTimeMult = RE::BSTimer::QGlobalTimeMultiplier();
            if (auto* calendar = RE::Calendar::GetSingleton()) {
                if (calendar->timeScale) g_snapshot.timescale = calendar->timeScale->value;
                if (calendar->gameHour) g_snapshot.gameHour = calendar->gameHour->value;
            }
            auto* sky = RE::Sky::GetSingleton();
            g_snapshot.weather = sky ? sky->currentWeather : nullptr;
        }

        if (!g_snapshot.freeCamera) {
            camera->ToggleFreeCameraMode(false);
            if (!IsFreeCamera(camera)) {
                spdlog::warn("PrismaPhotoMode: free camera did not engage; photo mode not entered");
                return false;
            }
        }
        main->freezeTime = true;
        saves->savingAllowed = false;
        g_timescaleMult = 1.0F;
        g_weatherForced = false;
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
        ReleaseWeather();
        RestoreTime();
        ResetDepthOfField();
        ResetFilter();
        SetPickedFrozen(false);
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

    void SetFrozen(bool a_frozen)
    {
        if (!g_active) return;
        if (auto* main = RE::Main::GetSingleton()) main->freezeTime = a_frozen;
    }

    bool Frozen() noexcept
    {
        const auto* main = RE::Main::GetSingleton();
        return main && main->freezeTime;
    }

    void SetTimeMultiplier(float a_multiplier)
    {
        if (!g_active || !RuntimeSupport::TimeControlsAvailable()) return;
        if (auto* timer = RE::BSTimer::GetSingleton()) timer->SetGlobalTimeMultiplier(a_multiplier, true);
    }

    float TimeMultiplier() noexcept
    {
        return RuntimeSupport::TimeControlsAvailable() ? RE::BSTimer::QGlobalTimeMultiplier() : 1.0F;
    }

    void SetGameHour(float a_hour)
    {
        if (!g_active || !RuntimeSupport::TimeControlsAvailable()) return;
        if (auto* calendar = RE::Calendar::GetSingleton(); calendar && calendar->gameHour) calendar->gameHour->value = a_hour;
    }

    float GameHour() noexcept
    {
        if (!RuntimeSupport::TimeControlsAvailable()) return 0.0F;
        const auto* calendar = RE::Calendar::GetSingleton();
        return calendar && calendar->gameHour ? calendar->gameHour->value : 0.0F;
    }

    void SetTimescaleMultiplier(float a_multiplier)
    {
        if (!g_active || !RuntimeSupport::TimeControlsAvailable()) return;
        g_timescaleMult = a_multiplier;
        if (auto* calendar = RE::Calendar::GetSingleton(); calendar && calendar->timeScale) {
            calendar->timeScale->value = g_snapshot.timescale * a_multiplier;
        }
    }

    float TimescaleMultiplier() noexcept
    {
        return g_timescaleMult;
    }

    void SetWeather(std::uint32_t a_formID)
    {
        if (!g_active || !RuntimeSupport::TimeControlsAvailable()) return;
        auto* sky = RE::Sky::GetSingleton();
        auto* weather = FindWeather(a_formID);
        if (!sky || (a_formID && !weather)) {
            spdlog::warn("PrismaPhotoMode: weather {:X} is not a known TESWeather", a_formID);
            return;
        }
        if (!weather) {
            ReleaseWeather();
            return;
        }
        sky->ForceWeather(weather, true);
        g_weatherForced = true;
    }

    void SetDepthOfField(bool a_on)
    {
        g_dofOn = a_on;
        ApplyDepthOfField();
    }

    void SetDepthOfFieldDistance(float a_distance)
    {
        g_dofDistance = a_distance;
        ApplyDepthOfField();
    }

    void SetAutoFocus(bool a_on)
    {
        g_autoFocus = a_on;
        ApplyDepthOfField();
    }

    void SetFilter(std::uint32_t a_formID)
    {
        if (!g_active || !RuntimeSupport::FiltersAvailable()) return;
        StopFilter();
        if (!a_formID) return;
        g_filter = FindForm<RE::TESImageSpaceModifier>(a_formID);
        if (!g_filter) {
            spdlog::warn("PrismaPhotoMode: image space modifier {:X} is not a known form", a_formID);
            return;
        }
        RE::ImageSpaceModifierInstanceForm::Trigger(g_filter, g_filterStrength, nullptr);
    }

    void SetFilterStrength(float a_strength)
    {
        g_filterStrength = a_strength;
        if (!g_filter) return;
        RE::ImageSpaceModifierInstanceForm::Stop(g_filter);
        RE::ImageSpaceModifierInstanceForm::Trigger(g_filter, g_filterStrength, nullptr);
    }

    void SetPickedFrozen(bool a_on)
    {
        if (!RuntimeSupport::CharactersAvailable()) return;
        if (a_on) {
            const auto ref = PickedRef();
            auto* actor = ref ? ref->As<RE::Actor>() : nullptr;
            if (!actor) return;
            actor->InitiateDoNothingPackage();
            g_frozen = actor->GetHandle();
            return;
        }
        const auto ref = g_frozen.get();
        auto* actor = ref ? ref->As<RE::Actor>() : nullptr;
        g_frozen = RE::ObjectRefHandle{};
        if (!actor) return;
        actor->EndInterruptPackage(false);
        if (actor->currentProcess) actor->currentProcess->StopCurrentIdle(actor, true, false);
    }

    void FacePicked()
    {
        if (!RuntimeSupport::CharactersAvailable()) return;
        const auto ref = PickedRef();
        auto* actor = ref ? ref->As<RE::Actor>() : nullptr;
        const auto* camera = RE::PlayerCamera::GetSingleton();
        RE::NiPoint3 cameraPosition{};
        if (!actor || !camera || !camera->GetCameraPosition(cameraPosition, true)) return;
        const auto delta = cameraPosition - actor->GetPosition();
        actor->SetHeading(std::atan2(delta.x, delta.y));
    }

    void PlayIdleOnPick(std::uint32_t a_formID)
    {
        if (!RuntimeSupport::CharactersAvailable()) return;
        const auto ref = PickedRef();
        auto* actor = ref ? ref->As<RE::Actor>() : nullptr;
        auto* idle = FindForm<RE::TESIdleForm>(a_formID);
        if (!actor || !idle || !actor->currentProcess) return;
        actor->currentProcess->PlayIdle(*actor, idle, nullptr);
    }

    void ApplyArtToPick(std::uint32_t a_formID)
    {
        if (!RuntimeSupport::CharactersAvailable()) return;
        const auto ref = PickedRef();
        auto* art = FindForm<RE::BGSArtObject>(a_formID);
        if (ref && art) ref->ApplyArtObject(art);
    }

    void ApplyShaderToPick(std::uint32_t a_formID)
    {
        if (!RuntimeSupport::CharactersAvailable()) return;
        const auto ref = PickedRef();
        auto* shader = FindForm<RE::TESEffectShader>(a_formID);
        if (ref && shader) ref->ApplyEffectShader(shader);
    }

    const char* PickedName()
    {
        if (!RuntimeSupport::CharactersAvailable()) return nullptr;
        const auto ref = PickedRef();
        return ref ? ref->GetDisplayFullName() : nullptr;
    }
}
