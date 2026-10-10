#pragma once

#include "hid/unique_handle.h"

#include <hidsdi.h>
#include <hidpi.h>

#include <string>
#include <vector>

namespace ps3translator::hid
{
    // Informações de uma interface HID encontrada no sistema.
    struct DeviceInfo
    {
        std::wstring path;
        HIDD_ATTRIBUTES attributes{ sizeof(HIDD_ATTRIBUTES) };
        HIDP_CAPS caps{};
        std::wstring manufacturer;
        std::wstring product;
        bool canOpen = false;
    };

    // Converte um código de erro do Windows em texto legível.
    [[nodiscard]] std::wstring FormatWindowsError(DWORD error);

    // Abre um dispositivo HID (leitura/escrita, com fallback para somente leitura).
    [[nodiscard]] UniqueHandle OpenHidDevice(const std::wstring& path, DWORD flags = 0);

    // Lista todas as interfaces HID presentes.
    [[nodiscard]] std::vector<DeviceInfo> EnumerateHidDevices();

    // True para joysticks/gamepads (Usage Page 0x01, Usage 0x04/0x05).
    [[nodiscard]] bool IsGameController(const DeviceInfo& device);

    // Nome amigável para exibir em listas, ex.: "Controle (054C:0268)".
    [[nodiscard]] std::wstring DisplayName(const DeviceInfo& device);
}
