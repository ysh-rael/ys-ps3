#pragma once

#include "calibration/device_profile.h"
#include "core/report_bytes.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ps3translator
{
    enum class CalibrationPhase
    {
        CapturingRest,   // mede o estado de repouso e detecta bytes "ruidosos" (sensores)
        WaitingRelease,  // espera o usuário soltar tudo antes do próximo controle
        WaitingInput,    // espera o usuário acionar o controle pedido
        Settling,        // coleta amostras enquanto o usuário segura o controle
        Finished
    };

    struct CalibrationStatus
    {
        CalibrationPhase phase = CalibrationPhase::CapturingRest;
        std::size_t stepIndex = 0;
        std::size_t stepCount = kControlCount;
        std::wstring prompt;   // instrução principal
        std::wstring message;  // resultado do último passo / dica
    };

    // Máquina de estados que descobre, a partir de reports brutos, quais bytes/bits
    // correspondem a cada controle Xbox. Não depende de Windows nem de threads;
    // quem chama é responsável por serializar o acesso.
    class Calibrator
    {
    public:
        using Clock = std::chrono::steady_clock;

        Calibrator(std::uint16_t vendorId, std::uint16_t productId);

        void Begin();
        void Feed(const ReportBytes& report, Clock::time_point now);
        void Skip();  // pula o controle atual sem mapeá-lo

        [[nodiscard]] CalibrationStatus Status() const;
        [[nodiscard]] bool IsFinished() const { return phase_ == CalibrationPhase::Finished; }
        [[nodiscard]] const DeviceProfile& Profile() const { return profile_; }

    private:
        struct Candidate
        {
            std::size_t index;
            std::uint8_t rest;
            std::uint8_t held;   // valor mais frequente enquanto segurado
            std::uint8_t peak;   // valor mais distante do repouso
            std::uint32_t hits;  // amostras fora do repouso
        };

        [[nodiscard]] bool ByteDeviates(std::size_t index, std::uint8_t value) const;
        [[nodiscard]] bool HasDeviation(const ReportBytes& report) const;
        void FinishRestCapture(Clock::time_point now);
        void Accumulate(const ReportBytes& report);
        void BeginStep(Clock::time_point now);
        void AdvanceStep(Clock::time_point now);
        void Claim(const Binding& binding);

        [[nodiscard]] std::vector<Candidate> Candidates() const;
        [[nodiscard]] std::optional<Binding> Detect(ControlKind kind) const;
        [[nodiscard]] std::optional<Binding> DetectBit(const std::vector<Candidate>& candidates, bool singleBitOnly) const;
        [[nodiscard]] std::optional<Binding> DetectHat(const std::vector<Candidate>& candidates) const;
        [[nodiscard]] static std::optional<Binding> DetectAnalog(const std::vector<Candidate>& candidates, bool skipBitLike);

        DeviceProfile profile_;
        CalibrationPhase phase_ = CalibrationPhase::CapturingRest;
        std::size_t step_ = 0;
        Clock::time_point phaseStart_{};
        bool restStarted_ = false;
        Clock::time_point lastTime_{};

        ReportBytes rest_;     // report de repouso
        ReportBytes minSeen_;  // mínimos/máximos vistos em repouso
        ReportBytes maxSeen_;
        ReportBytes peak_;     // valor mais distante do repouso no passo atual
        std::vector<std::uint8_t> noisy_;    // byte oscila em repouso (ignorado)
        std::vector<std::uint8_t> stable_;   // byte nunca mudou em repouso
        std::vector<std::uint8_t> claimed_;  // bits já usados por outro controle
        std::vector<std::uint32_t> counts_;  // histograma [byte][valor] do passo atual
        std::wstring message_;
    };
}
