#include "core/input_mapping.h"

#include <algorithm>
#include <cmath>

namespace ps3translator
{
    float NormalizeUnsignedAxis(unsigned char rawValue)
    {
        constexpr float center = 127.5F;
        return std::clamp((static_cast<float>(rawValue) - center) / center, -1.0F, 1.0F);
    }

    float ApplyAxisSettings(float value, const AxisSettings& settings)
    {
        const float clampedValue = std::clamp(value, -1.0F, 1.0F);
        const float deadzone = std::clamp(settings.deadzone, 0.0F, 0.99F);
        const float sensitivity = std::max(settings.sensitivity, 0.0F);
        const float magnitude = std::abs(clampedValue);
        if (magnitude <= deadzone)
        {
            return 0.0F;
        }

        const float rescaledMagnitude = (magnitude - deadzone) / (1.0F - deadzone);
        const float adjustedValue = std::copysign(std::min(rescaledMagnitude * sensitivity, 1.0F), clampedValue);
        return settings.inverted ? -adjustedValue : adjustedValue;
    }

    float NormalizeTrigger(unsigned char rawValue)
    {
        return static_cast<float>(rawValue) / 255.0F;
    }
}