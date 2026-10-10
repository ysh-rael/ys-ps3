#pragma once

#include "calibration/device_profile.h"
#include "core/gamepad_state.h"
#include "core/report_bytes.h"

namespace ps3translator
{
    // Converte um report bruto em GamepadState usando o perfil calibrado.
    [[nodiscard]] GamepadState TranslateReport(const DeviceProfile& profile, const ReportBytes& report);
}
