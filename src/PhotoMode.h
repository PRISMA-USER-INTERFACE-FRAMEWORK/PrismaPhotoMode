#pragma once

#include <cstdint>

namespace PPM::PhotoMode
{
    [[nodiscard]] bool Active() noexcept;
    [[nodiscard]] bool Enter();
    void Exit();
    [[nodiscard]] bool InGameplayContext();
    void SetFOV(float a_fov);
    void SetTranslateSpeed(float a_speed);
    void ResetCamera();
    [[nodiscard]] float CurrentFOV() noexcept;
    [[nodiscard]] float CurrentTranslateSpeed() noexcept;

    void SetFrozen(bool a_frozen);
    [[nodiscard]] bool Frozen() noexcept;
    void SetTimeMultiplier(float a_multiplier);
    [[nodiscard]] float TimeMultiplier() noexcept;
    void SetGameHour(float a_hour);
    [[nodiscard]] float GameHour() noexcept;
    void SetTimescaleMultiplier(float a_multiplier);
    [[nodiscard]] float TimescaleMultiplier() noexcept;
    void SetWeather(std::uint32_t a_formID);

    void SetDepthOfField(bool a_on);
    void SetDepthOfFieldDistance(float a_distance);
    void SetDepthOfFieldStrength(float a_strength);
    void SetDepthOfFieldRange(float a_range);
    void SetAutoFocus(bool a_on);

    void SetFilter(std::uint32_t a_formID);
    void SetFilterStrength(float a_strength);

    void ResetLighting();
    [[nodiscard]] bool LightingActive() noexcept;
    void SetBrightness(float a_value);
    void SetSaturation(float a_value);
    void SetContrast(float a_value);
    [[nodiscard]] float Brightness() noexcept;
    [[nodiscard]] float Saturation() noexcept;
    [[nodiscard]] float Contrast() noexcept;

    void SetPlayerVisible(bool a_visible);
    void SetPickedFrozen(bool a_on);
    void FacePicked();
    void PlayIdleOnPick(std::uint32_t a_formID);
    void PlayIdleOnPlayer(std::uint32_t a_formID);
    void ApplyArtToPick(std::uint32_t a_formID);
    void ApplyShaderToPick(std::uint32_t a_formID);
    void SetExpressionOnPick(std::uint32_t a_formID);
    void SetExpressionOnPlayer(std::uint32_t a_formID);
    [[nodiscard]] const char* PickedName();

    void InstallViewRoll();
    void SetViewRoll(float a_degrees);
    [[nodiscard]] float ViewRoll() noexcept;
}
