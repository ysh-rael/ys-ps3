#include "hid/hid_reader.h"

#include <utility>

namespace ps3translator::hid
{
    bool HidReader::Start(const DeviceInfo& device, ReportHandler onReport, ClosedHandler onClosed, std::wstring& error)
    {
        Stop();

        if (device.caps.InputReportByteLength == 0)
        {
            error = L"O dispositivo não expõe Input Reports.";
            return false;
        }

        device_ = OpenHidDevice(device.path, FILE_FLAG_OVERLAPPED);
        if (!device_.valid())
        {
            error = L"Não foi possível abrir o dispositivo: " + FormatWindowsError(GetLastError());
            return false;
        }

        stopEvent_.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!stopEvent_.valid())
        {
            error = L"CreateEventW falhou: " + FormatWindowsError(GetLastError());
            device_.reset();
            return false;
        }

        stopRequested_ = false;
        thread_ = std::thread(&HidReader::Run, this, std::move(onReport), std::move(onClosed),
                              static_cast<size_t>(device.caps.InputReportByteLength));
        return true;
    }

    void HidReader::Stop()
    {
        if (thread_.joinable())
        {
            stopRequested_ = true;
            SetEvent(stopEvent_.get());
            thread_.join();
        }
        device_.reset();
        stopEvent_.reset();
    }

    void HidReader::Run(ReportHandler onReport, ClosedHandler onClosed, size_t reportLength)
    {
        UniqueHandle readEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!readEvent.valid())
        {
            if (onClosed) onClosed();
            return;
        }

        ReportBytes buffer(reportLength);
        const HANDLE waits[2] = { readEvent.get(), stopEvent_.get() };

        for (;;)
        {
            OVERLAPPED overlapped{};
            overlapped.hEvent = readEvent.get();
            ResetEvent(readEvent.get());

            DWORD bytesRead = 0;
            if (!ReadFile(device_.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, &overlapped))
            {
                if (GetLastError() != ERROR_IO_PENDING)
                {
                    break;
                }

                // Espera um report novo ou o pedido de parada.
                if (WaitForMultipleObjects(2, waits, FALSE, INFINITE) != WAIT_OBJECT_0)
                {
                    CancelIoEx(device_.get(), &overlapped);
                    GetOverlappedResult(device_.get(), &overlapped, &bytesRead, TRUE);
                    break;
                }
                if (!GetOverlappedResult(device_.get(), &overlapped, &bytesRead, FALSE))
                {
                    break;
                }
            }

            if (bytesRead > 0 && onReport)
            {
                onReport(ReportBytes(buffer.begin(), buffer.begin() + bytesRead));
            }
        }

        if (!stopRequested_ && onClosed)
        {
            onClosed();
        }
    }
}
