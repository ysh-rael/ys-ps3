#include "hid/hid_device.h"

#include <setupapi.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <utility>

namespace ps3translator::hid
{
    namespace
    {
        std::optional<std::wstring> GetHidString(HANDLE handle, BOOLEAN(WINAPI* getString)(HANDLE, PVOID, ULONG))
        {
            wchar_t buffer[256]{};
            if (!getString(handle, buffer, sizeof(buffer)))
            {
                return std::nullopt;
            }
            return std::wstring(buffer);
        }
    }

    std::wstring FormatWindowsError(DWORD error)
    {
        LPWSTR message = nullptr;
        const DWORD length = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            0,
            reinterpret_cast<LPWSTR>(&message),
            0,
            nullptr);

        std::wstringstream result;
        result << L"erro " << error;
        if (length != 0 && message != nullptr)
        {
            std::wstring text(message, length);
            LocalFree(message);
            text.erase(std::remove(text.begin(), text.end(), L'\r'), text.end());
            text.erase(std::remove(text.begin(), text.end(), L'\n'), text.end());
            result << L" (" << text << L")";
        }
        return result.str();
    }

    UniqueHandle OpenHidDevice(const std::wstring& path, DWORD flags)
    {
        HANDLE handle = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            flags,
            nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            handle = CreateFileW(
                path.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                flags,
                nullptr);
        }
        return UniqueHandle(handle);
    }

    std::vector<DeviceInfo> EnumerateHidDevices()
    {
        GUID hidGuid{};
        HidD_GetHidGuid(&hidGuid);

        const HDEVINFO deviceSet = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (deviceSet == INVALID_HANDLE_VALUE)
        {
            return {};
        }

        std::vector<DeviceInfo> devices;
        for (DWORD index = 0;; ++index)
        {
            SP_DEVICE_INTERFACE_DATA interfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };
            if (!SetupDiEnumDeviceInterfaces(deviceSet, nullptr, &hidGuid, index, &interfaceData))
            {
                break;
            }

            // Primeira chamada apenas descobre o tamanho do buffer do caminho.
            DWORD requiredBytes = 0;
            SetupDiGetDeviceInterfaceDetailW(deviceSet, &interfaceData, nullptr, 0, &requiredBytes, nullptr);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || requiredBytes < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))
            {
                continue;
            }

            auto rawDetail = std::make_unique<std::byte[]>(requiredBytes);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(rawDetail.get());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(deviceSet, &interfaceData, detail, requiredBytes, nullptr, nullptr))
            {
                continue;
            }

            DeviceInfo info{};
            info.path = detail->DevicePath;
            UniqueHandle handle = OpenHidDevice(info.path);
            if (!handle.valid())
            {
                devices.push_back(std::move(info));
                continue;
            }

            info.canOpen = true;
            HidD_GetAttributes(handle.get(), &info.attributes);

            if (const auto manufacturer = GetHidString(handle.get(), HidD_GetManufacturerString))
            {
                info.manufacturer = *manufacturer;
            }
            if (const auto product = GetHidString(handle.get(), HidD_GetProductString))
            {
                info.product = *product;
            }

            PHIDP_PREPARSED_DATA preparsedData = nullptr;
            if (HidD_GetPreparsedData(handle.get(), &preparsedData))
            {
                HidP_GetCaps(preparsedData, &info.caps);
                HidD_FreePreparsedData(preparsedData);
            }

            devices.push_back(std::move(info));
        }

        SetupDiDestroyDeviceInfoList(deviceSet);
        return devices;
    }

    bool IsGameController(const DeviceInfo& device)
    {
        return device.caps.UsagePage == 0x01 && (device.caps.Usage == 0x04 || device.caps.Usage == 0x05);
    }

    std::wstring DisplayName(const DeviceInfo& device)
    {
        std::wstringstream name;
        name << (device.product.empty() ? L"Dispositivo HID" : device.product)
             << L" (" << std::hex << std::uppercase << std::setw(4) << std::setfill(L'0') << device.attributes.VendorID
             << L':' << std::setw(4) << device.attributes.ProductID << L')';
        return name.str();
    }
}
