#pragma once

#include "core/report_bytes.h"
#include "hid/hid_device.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace ps3translator::hid
{
    // Lê Input Reports de um dispositivo HID em uma thread própria.
    // O primeiro byte do report é o Report ID (0 quando o dispositivo não usa IDs).
    class HidReader
    {
    public:
        using ReportHandler = std::function<void(const ReportBytes&)>;
        using ClosedHandler = std::function<void()>;

        HidReader() = default;
        ~HidReader() { Stop(); }

        HidReader(const HidReader&) = delete;
        HidReader& operator=(const HidReader&) = delete;

        // onReport roda na thread de leitura; onClosed é chamado se o dispositivo falhar/desconectar.
        [[nodiscard]] bool Start(const DeviceInfo& device, ReportHandler onReport, ClosedHandler onClosed, std::wstring& error);

        // Para a leitura e aguarda a thread terminar (seguro chamar mais de uma vez).
        void Stop();

    private:
        void Run(ReportHandler onReport, ClosedHandler onClosed, size_t reportLength);

        std::thread thread_;
        UniqueHandle device_;
        UniqueHandle stopEvent_;
        std::atomic<bool> stopRequested_{ false };
    };
}
