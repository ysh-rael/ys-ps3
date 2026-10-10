#pragma once

#include "calibration/device_profile.h"

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>

namespace ps3translator
{
    // Serialização em texto simples (uma linha por controle). Usada também nos testes.
    void WriteProfile(std::ostream& out, const DeviceProfile& profile);
    [[nodiscard]] std::optional<DeviceProfile> ReadProfile(std::istream& in);

    // Pasta dos perfis: %APPDATA%\PS3Translator\profiles.
    [[nodiscard]] std::filesystem::path ProfilesDirectory();
    [[nodiscard]] std::filesystem::path ProfilePath(std::uint16_t vendorId, std::uint16_t productId);

    [[nodiscard]] bool ProfileExists(std::uint16_t vendorId, std::uint16_t productId);
    [[nodiscard]] bool SaveProfile(const DeviceProfile& profile, std::wstring& error);
    [[nodiscard]] std::optional<DeviceProfile> LoadProfile(std::uint16_t vendorId, std::uint16_t productId);
}
