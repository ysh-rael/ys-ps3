#include "hid/hid_device.h"

#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

// Ferramenta de console: lista dispositivos HID e mostra os reports brutos (--watch).
namespace
{
    using namespace ps3translator::hid;

    void PrintDevice(const DeviceInfo& device, size_t index)
    {
        std::wcout << L"[" << index << L"] Device\n"
                   << L"  VID: 0x" << std::hex << std::setw(4) << std::setfill(L'0') << device.attributes.VendorID << L'\n'
                   << L"  PID: 0x" << std::setw(4) << device.attributes.ProductID << std::dec << std::setfill(L' ') << L'\n'
                   << L"  Manufacturer: " << (device.manufacturer.empty() ? L"(unavailable)" : device.manufacturer) << L'\n'
                   << L"  Product: " << (device.product.empty() ? L"(unavailable)" : device.product) << L'\n'
                   << L"  Usage Page: 0x" << std::hex << std::setw(4) << std::setfill(L'0') << device.caps.UsagePage << L'\n'
                   << L"  Usage: 0x" << std::setw(4) << device.caps.Usage << std::dec << std::setfill(L' ') << L'\n'
                   << L"  Input report bytes: " << device.caps.InputReportByteLength << L'\n'
                   << L"  Path: " << device.path << L'\n'
                   << L"  Openable: " << (device.canOpen ? L"yes" : L"no") << L"\n\n";
    }

    void PrintReport(const std::vector<std::byte>& report, const std::vector<std::byte>& previous)
    {
        std::wcout << L"Input report (" << report.size() << L" bytes): ";
        for (const std::byte value : report)
        {
            std::wcout << std::hex << std::setw(2) << std::setfill(L'0') << std::to_integer<unsigned int>(value) << L' ';
        }
        std::wcout << std::dec << std::setfill(L' ') << L'\n';

        if (!previous.empty())
        {
            std::wcout << L"Changed bytes:";
            bool changed = false;
            for (size_t index = 0; index < report.size(); ++index)
            {
                if (index >= previous.size() || report[index] != previous[index])
                {
                    const unsigned int previousValue = index < previous.size()
                        ? std::to_integer<unsigned int>(previous[index])
                        : 0;
                    std::wcout << L" [" << index << L"] "
                               << std::hex << std::setw(2) << std::setfill(L'0') << previousValue
                               << L"->" << std::setw(2) << std::to_integer<unsigned int>(report[index]) << std::dec << std::setfill(L' ');
                    changed = true;
                }
            }
            if (!changed)
            {
                std::wcout << L" none";
            }
            std::wcout << L'\n';
        }
        std::wcout << L'\n';
    }

    int WatchDevice(const DeviceInfo& device)
    {
        if (device.caps.InputReportByteLength == 0)
        {
            std::wcerr << L"The selected device does not expose input reports.\n";
            return 1;
        }

        UniqueHandle deviceHandle = OpenHidDevice(device.path, FILE_FLAG_OVERLAPPED);
        if (!deviceHandle.valid())
        {
            std::wcerr << L"Cannot open selected HID device: " << FormatWindowsError(GetLastError()) << L'\n';
            return 1;
        }

        UniqueHandle eventHandle(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!eventHandle.valid())
        {
            std::wcerr << L"CreateEventW failed: " << FormatWindowsError(GetLastError()) << L'\n';
            return 1;
        }

        std::vector<std::byte> report(device.caps.InputReportByteLength);
        std::vector<std::byte> previous;
        std::wcout << L"Watching device. Press Ctrl+C to stop.\n\n";

        for (;;)
        {
            OVERLAPPED overlapped{};
            overlapped.hEvent = eventHandle.get();
            ResetEvent(eventHandle.get());

            DWORD bytesRead = 0;
            if (!ReadFile(deviceHandle.get(), report.data(), static_cast<DWORD>(report.size()), &bytesRead, &overlapped))
            {
                const DWORD error = GetLastError();
                if (error != ERROR_IO_PENDING)
                {
                    std::wcerr << L"ReadFile failed: " << FormatWindowsError(error) << L'\n';
                    return 1;
                }
                if (!GetOverlappedResult(deviceHandle.get(), &overlapped, &bytesRead, TRUE))
                {
                    std::wcerr << L"GetOverlappedResult failed: " << FormatWindowsError(GetLastError()) << L'\n';
                    return 1;
                }
            }

            report.resize(bytesRead);
            PrintReport(report, previous);
            previous = report;
            report.resize(device.caps.InputReportByteLength);
        }
    }

    void PrintUsage()
    {
        std::wcout << L"Usage:\n"
                   << L"  PS3Translator.exe --list\n"
                   << L"  PS3Translator.exe --watch <device-index>\n";
    }
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc < 2 || std::wstring_view(argv[1]) == L"--help")
    {
        PrintUsage();
        return 0;
    }

    const std::vector<DeviceInfo> devices = EnumerateHidDevices();
    if (std::wstring_view(argv[1]) == L"--list")
    {
        for (size_t index = 0; index < devices.size(); ++index)
        {
            PrintDevice(devices[index], index);
        }
        return 0;
    }

    if (std::wstring_view(argv[1]) == L"--watch" && argc == 3)
    {
        try
        {
            const size_t index = std::stoull(argv[2]);
            if (index >= devices.size())
            {
                std::wcerr << L"Invalid device index. Run --list first.\n";
                return 1;
            }
            return WatchDevice(devices[index]);
        }
        catch (const std::exception&)
        {
            std::wcerr << L"Device index must be an unsigned integer.\n";
            return 1;
        }
    }

    PrintUsage();
    return 1;
}
