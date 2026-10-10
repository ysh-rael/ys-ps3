#include "app/settings.h"

#include <windows.h>

#include <string>

namespace ps3translator::settings
{
    namespace
    {
        constexpr wchar_t kAppKey[] = L"Software\\ysh-rael\\ys-ps3";
        constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        constexpr wchar_t kRunValue[] = L"ys-ps3";
    }

    bool GetAutoBridge()
    {
        DWORD value = 1;
        DWORD size = sizeof(value);
        RegGetValueW(HKEY_CURRENT_USER, kAppKey, L"AutoBridge", RRF_RT_REG_DWORD, nullptr, &value, &size);
        return value != 0;
    }

    void SetAutoBridge(bool enabled)
    {
        HKEY key = nullptr;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, kAppKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
        {
            const DWORD value = enabled ? 1 : 0;
            RegSetValueExW(key, L"AutoBridge", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
            RegCloseKey(key);
        }
    }

    bool GetRunAtStartup()
    {
        return RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    }

    void SetRunAtStartup(bool enabled)
    {
        HKEY key = nullptr;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        {
            return;
        }
        if (enabled)
        {
            wchar_t path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, path, MAX_PATH);
            const std::wstring command = L"\"" + std::wstring(path) + L"\" --background";
            RegSetValueExW(key, kRunValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(command.c_str()),
                           static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
        }
        else
        {
            RegDeleteValueW(key, kRunValue);
        }
        RegCloseKey(key);
    }
}
