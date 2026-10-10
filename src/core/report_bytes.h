#pragma once

#include <cstdint>
#include <vector>

namespace ps3translator
{
    // Bytes brutos de um Input Report HID (byte 0 = Report ID).
    using ReportBytes = std::vector<std::uint8_t>;
}
