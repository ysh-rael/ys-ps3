#pragma once

namespace ps3translator
{
    struct GamepadState
    {
        float leftX = 0.0F;
        float leftY = 0.0F;
        float rightX = 0.0F;
        float rightY = 0.0F;

        float leftTrigger = 0.0F;
        float rightTrigger = 0.0F;

        bool a = false;
        bool b = false;
        bool x = false;
        bool y = false;

        bool dpadUp = false;
        bool dpadDown = false;
        bool dpadLeft = false;
        bool dpadRight = false;

        bool leftShoulder = false;
        bool rightShoulder = false;

        bool start = false;
        bool back = false;
        bool guide = false;

        bool leftStick = false;
        bool rightStick = false;
    };
}