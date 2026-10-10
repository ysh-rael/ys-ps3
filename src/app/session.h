#pragma once

#include "calibration/calibrator.h"
#include "calibration/device_profile.h"
#include "hid/hid_device.h"
#include "hid/hid_reader.h"
#include "xbox/virtual_xbox.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace ps3translator
{
    enum class SessionMode
    {
        Idle,
        Calibrating,
        Bridging  // lendo o controle físico e alimentando o Xbox virtual
    };

    struct SessionStatus
    {
        SessionMode mode = SessionMode::Idle;
        CalibrationStatus calibration;  // válido apenas em Calibrating
        std::wstring message;           // mensagem geral (erros, conclusão, etc.)
    };

    // Coordena leitura HID, calibração e ponte para o Xbox virtual.
    // Os métodos públicos são chamados pela thread da interface.
    class Session
    {
    public:
        Session() = default;
        ~Session() { Stop(); }

        // Chamado em outra thread quando o dispositivo some; a UI deve responder chamando Stop().
        std::function<void()> onDeviceLost;

        [[nodiscard]] bool StartCalibration(const hid::DeviceInfo& device);
        [[nodiscard]] bool StartBridge(const hid::DeviceInfo& device);
        void SkipStep();
        void Stop(const std::wstring& message = L"");

        // Deve ser chamado periodicamente pela UI: conclui a calibração e devolve o estado atual.
        [[nodiscard]] SessionStatus Poll();

    private:
        void FinishCalibration();

        std::mutex mutex_;
        hid::HidReader reader_;
        std::unique_ptr<Calibrator> calibrator_;
        DeviceProfile profile_;
        VirtualXbox xbox_;
        SessionMode mode_ = SessionMode::Idle;
        std::wstring message_;
    };
}
