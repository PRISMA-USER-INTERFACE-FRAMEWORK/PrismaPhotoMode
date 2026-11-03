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
    bool QueueScreenshot();

    void SetFrozen(bool a_frozen);
    [[nodiscard]] bool Frozen() noexcept;
    void SetTimeMultiplier(float a_multiplier);
    [[nodiscard]] float TimeMultiplier() noexcept;
    void SetGameHour(float a_hour);
    [[nodiscard]] float GameHour() noexcept;
    void SetTimescaleMultiplier(float a_multiplier);
    [[nodiscard]] float TimescaleMultiplier() noexcept;
    void SetWeather(std::uint32_t a_formID);
}
