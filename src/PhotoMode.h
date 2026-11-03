#pragma once

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
}
