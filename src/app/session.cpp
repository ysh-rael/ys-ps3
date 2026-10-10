#include "app/session.h"

#include "calibration/profile_store.h"
#include "translator/report_translator.h"

namespace ps3translator
{
    bool Session::StartCalibration(const hid::DeviceInfo& device)
    {
        Stop();

        auto calibrator = std::make_unique<Calibrator>(device.attributes.VendorID, device.attributes.ProductID);
        calibrator->Begin();
        {
            std::lock_guard lock(mutex_);
            calibrator_ = std::move(calibrator);
            mode_ = SessionMode::Calibrating;
            message_.clear();
        }

        std::wstring error;
        const bool started = reader_.Start(
            device,
            [this](const ReportBytes& report)
            {
                std::lock_guard lock(mutex_);
                if (calibrator_)
                {
                    calibrator_->Feed(report, Calibrator::Clock::now());
                }
            },
            [this] { if (onDeviceLost) onDeviceLost(); },
            error);

        if (!started)
        {
            Stop(error);
        }
        return started;
    }

    bool Session::StartBridge(const hid::DeviceInfo& device)
    {
        Stop();

        const auto profile = LoadProfile(device.attributes.VendorID, device.attributes.ProductID);
        if (!profile)
        {
            Stop(L"Este controle ainda não foi calibrado.");
            return false;
        }

        std::wstring error;
        if (!xbox_.Connect(error))
        {
            Stop(error);
            return false;
        }

        {
            std::lock_guard lock(mutex_);
            profile_ = *profile;
            mode_ = SessionMode::Bridging;
            message_ = L"Xbox virtual ativo. O Windows já o reconhece como controle Xbox 360.";
        }

        const bool started = reader_.Start(
            device,
            [this](const ReportBytes& report)
            {
                // Traduz e envia cada report; roda na thread de leitura para menor latência.
                std::lock_guard lock(mutex_);
                if (mode_ == SessionMode::Bridging)
                {
                    xbox_.Submit(TranslateReport(profile_, report));
                }
            },
            [this] { if (onDeviceLost) onDeviceLost(); },
            error);

        if (!started)
        {
            Stop(error);
        }
        return started;
    }

    void Session::SkipStep()
    {
        std::lock_guard lock(mutex_);
        if (calibrator_)
        {
            calibrator_->Skip();
        }
    }

    void Session::Stop(const std::wstring& message)
    {
        // A thread de leitura usa o mutex, então ela precisa terminar antes de o travarmos.
        reader_.Stop();

        std::lock_guard lock(mutex_);
        xbox_.Disconnect();
        calibrator_.reset();
        mode_ = SessionMode::Idle;
        message_ = message;
    }

    void Session::FinishCalibration()
    {
        reader_.Stop();

        std::lock_guard lock(mutex_);
        if (!calibrator_)
        {
            return;
        }
        std::wstring error;
        message_ = SaveProfile(calibrator_->Profile(), error)
            ? L"Calibração concluída e salva. Já é possível iniciar o Xbox virtual."
            : error;
        calibrator_.reset();
        mode_ = SessionMode::Idle;
    }

    SessionStatus Session::Poll()
    {
        bool finished = false;
        {
            std::lock_guard lock(mutex_);
            finished = mode_ == SessionMode::Calibrating && calibrator_ && calibrator_->IsFinished();
        }
        if (finished)
        {
            FinishCalibration();
        }

        std::lock_guard lock(mutex_);
        SessionStatus status;
        status.mode = mode_;
        status.message = message_;
        if (calibrator_)
        {
            status.calibration = calibrator_->Status();
        }
        return status;
    }
}
