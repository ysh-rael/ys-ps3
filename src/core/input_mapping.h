#pragma once

namespace ps3translator
{
    struct AxisSettings
    {
        float deadzone = 0.0F;
        float sensitivity = 1.0F;
        bool inverted = false;
    };

    [[nodiscard]] float NormalizeUnsignedAxis(unsigned char rawValue);
    [[nodiscard]] float ApplyAxisSettings(float value, const AxisSettings& settings);
    [[nodiscard]] float NormalizeTrigger(unsigned char rawValue);
}