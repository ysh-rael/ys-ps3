#include "core/input_mapping.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
    void ExpectNear(float actual, float expected, const char* description)
    {
        constexpr float tolerance = 0.0001F;
        if (std::abs(actual - expected) > tolerance)
        {
            std::cerr << description << ": expected " << expected << ", got " << actual << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    using namespace ps3translator;

    ExpectNear(NormalizeUnsignedAxis(0), -1.0F, "minimum axis value");
    ExpectNear(NormalizeUnsignedAxis(128), 1.0F / 255.0F, "axis center rounding");
    ExpectNear(NormalizeUnsignedAxis(255), 1.0F, "maximum axis value");
    ExpectNear(NormalizeTrigger(0), 0.0F, "minimum trigger value");
    ExpectNear(NormalizeTrigger(255), 1.0F, "maximum trigger value");

    ExpectNear(ApplyAxisSettings(0.15F, AxisSettings{ 0.2F }), 0.0F, "deadzone");
    ExpectNear(ApplyAxisSettings(0.6F, AxisSettings{ 0.2F }), 0.5F, "deadzone rescaling");
    ExpectNear(ApplyAxisSettings(-0.6F, AxisSettings{ 0.2F, 1.0F, true }), 0.5F, "inversion");
    ExpectNear(ApplyAxisSettings(0.8F, AxisSettings{ 0.0F, 2.0F }), 1.0F, "sensitivity clamp");

    return EXIT_SUCCESS;
}