#pragma once

#include "calibration/control_map.h"

#include <array>
#include <cstdint>

namespace ps3translator
{
    // Como um controle Xbox é lido a partir dos bytes do report.
    enum class BindingKind : std::uint8_t
    {
        None,    // não mapeado
        Bit,     // pressionado quando (report[byteIndex] & mask) == value
        Hat,     // hat switch (nibble baixo 0-7, sentido horário); value = direção capturada
        Analog   // valor proporcional entre rest (repouso) e full (extremo)
    };

    struct Binding
    {
        BindingKind kind = BindingKind::None;
        std::uint16_t byteIndex = 0;
        std::uint8_t mask = 0;
        std::uint8_t value = 0;
        std::uint8_t rest = 0;
        std::uint8_t full = 0;
    };

    // Resultado da calibração de um dispositivo (salvo em disco por VID/PID).
    struct DeviceProfile
    {
        std::uint16_t vendorId = 0;
        std::uint16_t productId = 0;
        std::array<Binding, kControlCount> bindings{};
    };
}
