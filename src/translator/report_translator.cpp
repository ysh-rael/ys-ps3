#include "translator/report_translator.h"

#include "core/input_mapping.h"

#include <algorithm>

namespace ps3translator
{
    namespace
    {
        // Lê um controle como número: 0/1 para Bit e Hat, fração repouso->extremo para Analog.
        float ReadValue(const Binding& binding, const ReportBytes& report)
        {
            if (binding.kind == BindingKind::None || binding.byteIndex >= report.size())
            {
                return 0.0F;
            }
            const std::uint8_t raw = report[binding.byteIndex];

            switch (binding.kind)
            {
            case BindingKind::Bit:
                return (raw & binding.mask) == binding.value ? 1.0F : 0.0F;

            case BindingKind::Hat:
            {
                // Direções 0-7 em sentido horário; a direção capturada também vale para seus vizinhos
                // (assim as diagonais acionam as duas direções).
                const int nibble = raw & 0x0F;
                if (nibble >= 8)
                {
                    return 0.0F;
                }
                const int offset = (nibble - binding.value + 8) % 8;
                return (offset == 0 || offset == 1 || offset == 7) ? 1.0F : 0.0F;
            }

            case BindingKind::Analog:
            {
                const float span = static_cast<float>(binding.full) - static_cast<float>(binding.rest);
                if (span == 0.0F)
                {
                    return 0.0F;
                }
                return (static_cast<float>(raw) - static_cast<float>(binding.rest)) / span;
            }

            default:
                return 0.0F;
            }
        }

        bool Pressed(const DeviceProfile& profile, ControlId id, const ReportBytes& report)
        {
            return ReadValue(profile.bindings[static_cast<std::size_t>(id)], report) > 0.5F;
        }

        float Axis(const DeviceProfile& profile, ControlId id, const ReportBytes& report)
        {
            constexpr AxisSettings settings{ 0.08F };
            const float value = std::clamp(ReadValue(profile.bindings[static_cast<std::size_t>(id)], report), -1.0F, 1.0F);
            return ApplyAxisSettings(value, settings);
        }

        float Trigger(const DeviceProfile& profile, ControlId id, const ReportBytes& report)
        {
            return std::clamp(ReadValue(profile.bindings[static_cast<std::size_t>(id)], report), 0.0F, 1.0F);
        }
    }

    GamepadState TranslateReport(const DeviceProfile& profile, const ReportBytes& report)
    {
        GamepadState state;
        state.a = Pressed(profile, ControlId::A, report);
        state.b = Pressed(profile, ControlId::B, report);
        state.x = Pressed(profile, ControlId::X, report);
        state.y = Pressed(profile, ControlId::Y, report);
        state.leftShoulder = Pressed(profile, ControlId::LeftShoulder, report);
        state.rightShoulder = Pressed(profile, ControlId::RightShoulder, report);
        state.back = Pressed(profile, ControlId::Back, report);
        state.start = Pressed(profile, ControlId::Start, report);
        state.guide = Pressed(profile, ControlId::Guide, report);
        state.leftStick = Pressed(profile, ControlId::LeftStick, report);
        state.rightStick = Pressed(profile, ControlId::RightStick, report);
        state.dpadUp = Pressed(profile, ControlId::DpadUp, report);
        state.dpadDown = Pressed(profile, ControlId::DpadDown, report);
        state.dpadLeft = Pressed(profile, ControlId::DpadLeft, report);
        state.dpadRight = Pressed(profile, ControlId::DpadRight, report);
        state.leftX = Axis(profile, ControlId::LeftStickX, report);
        state.leftY = Axis(profile, ControlId::LeftStickY, report);
        state.rightX = Axis(profile, ControlId::RightStickX, report);
        state.rightY = Axis(profile, ControlId::RightStickY, report);
        state.leftTrigger = Trigger(profile, ControlId::LeftTrigger, report);
        state.rightTrigger = Trigger(profile, ControlId::RightTrigger, report);
        return state;
    }
}
