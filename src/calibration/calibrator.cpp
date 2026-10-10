#include "calibration/calibrator.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace ps3translator
{
    namespace
    {
        using namespace std::chrono_literals;

        constexpr auto kRestDuration = 1500ms;     // tempo medindo o repouso
        constexpr auto kSettleDuration = 500ms;    // tempo coletando enquanto o controle é segurado
        constexpr auto kReleaseTimeout = 2500ms;   // limite esperando o usuário soltar tudo
        constexpr int kNoiseSpread = 6;            // oscilação máxima tolerada em repouso
        constexpr int kAnalogThreshold = 48;       // variação mínima para considerar um eixo/gatilho

        int Distance(std::uint8_t a, std::uint8_t b)
        {
            return std::abs(static_cast<int>(a) - static_cast<int>(b));
        }

        std::wstring Hex(unsigned int value)
        {
            std::wstringstream text;
            text << L"0x" << std::hex << std::uppercase << value;
            return text.str();
        }
    }

    Calibrator::Calibrator(std::uint16_t vendorId, std::uint16_t productId)
    {
        profile_.vendorId = vendorId;
        profile_.productId = productId;
    }

    void Calibrator::Begin()
    {
        phase_ = CalibrationPhase::CapturingRest;
        restStarted_ = false;
        step_ = 0;
        profile_.bindings.fill(Binding{});
        message_.clear();
    }

    bool Calibrator::ByteDeviates(std::size_t index, std::uint8_t value) const
    {
        if (noisy_[index])
        {
            return false;
        }

        // Ignora bits que já pertencem a outro controle.
        const auto keep = static_cast<std::uint8_t>(~claimed_[index]);
        const auto current = static_cast<std::uint8_t>(value & keep);
        const auto baseline = static_cast<std::uint8_t>(rest_[index] & keep);
        if (current == baseline)
        {
            return false;
        }
        // Bytes totalmente estáveis reagem a qualquer mudança; os demais exigem margem.
        return stable_[index] || Distance(current, baseline) > kNoiseSpread;
    }

    bool Calibrator::HasDeviation(const ReportBytes& report) const
    {
        for (std::size_t i = 0; i < report.size(); ++i)
        {
            if (ByteDeviates(i, report[i]))
            {
                return true;
            }
        }
        return false;
    }

    void Calibrator::FinishRestCapture(Clock::time_point now)
    {
        const std::size_t size = rest_.size();
        noisy_.assign(size, 0);
        stable_.assign(size, 0);
        claimed_.assign(size, 0);
        for (std::size_t i = 0; i < size; ++i)
        {
            const int spread = maxSeen_[i] - minSeen_[i];
            noisy_[i] = spread > kNoiseSpread;
            stable_[i] = spread == 0;
        }
        step_ = 0;
        BeginStep(now);
    }

    void Calibrator::BeginStep(Clock::time_point now)
    {
        counts_.assign(rest_.size() * 256, 0);
        peak_ = rest_;
        phase_ = CalibrationPhase::WaitingRelease;
        phaseStart_ = now;
    }

    void Calibrator::AdvanceStep(Clock::time_point now)
    {
        ++step_;
        if (step_ >= kControlCount)
        {
            phase_ = CalibrationPhase::Finished;
            return;
        }
        BeginStep(now);
    }

    void Calibrator::Skip()
    {
        if (phase_ == CalibrationPhase::WaitingRelease || phase_ == CalibrationPhase::WaitingInput
            || phase_ == CalibrationPhase::Settling)
        {
            message_ = L"Controle pulado.";
            AdvanceStep(lastTime_);
        }
    }

    void Calibrator::Feed(const ReportBytes& report, Clock::time_point now)
    {
        if (report.empty() || phase_ == CalibrationPhase::Finished)
        {
            return;
        }
        lastTime_ = now;

        if (phase_ == CalibrationPhase::CapturingRest)
        {
            if (!restStarted_)
            {
                restStarted_ = true;
                phaseStart_ = now;
                rest_ = minSeen_ = maxSeen_ = report;
                return;
            }
            if (report.size() != rest_.size())
            {
                return;
            }
            for (std::size_t i = 0; i < report.size(); ++i)
            {
                minSeen_[i] = std::min(minSeen_[i], report[i]);
                maxSeen_[i] = std::max(maxSeen_[i], report[i]);
            }
            rest_ = report;
            if (now - phaseStart_ >= kRestDuration)
            {
                FinishRestCapture(now);
            }
            return;
        }

        if (report.size() != rest_.size())
        {
            return;
        }

        switch (phase_)
        {
        case CalibrationPhase::WaitingRelease:
            if (!HasDeviation(report) || now - phaseStart_ >= kReleaseTimeout)
            {
                phase_ = CalibrationPhase::WaitingInput;
            }
            break;

        case CalibrationPhase::WaitingInput:
            if (HasDeviation(report))
            {
                phase_ = CalibrationPhase::Settling;
                phaseStart_ = now;
                Accumulate(report);
            }
            break;

        case CalibrationPhase::Settling:
            Accumulate(report);
            if (now - phaseStart_ >= kSettleDuration)
            {
                const auto kind = GetControlInfo(static_cast<ControlId>(step_)).kind;
                if (const auto binding = Detect(kind))
                {
                    profile_.bindings[step_] = *binding;
                    Claim(*binding);
                    std::wstringstream text;
                    text << L"Capturado: byte " << binding->byteIndex;
                    if (binding->kind == BindingKind::Bit)
                    {
                        text << L", máscara " << Hex(binding->mask);
                    }
                    message_ = text.str();
                    AdvanceStep(now);
                }
                else
                {
                    // Nada reconhecível: repete o mesmo controle.
                    message_ = L"Não foi possível identificar. Tente novamente ou use Pular.";
                    BeginStep(now);
                }
            }
            break;

        default:
            break;
        }
    }

    void Calibrator::Accumulate(const ReportBytes& report)
    {
        for (std::size_t i = 0; i < report.size(); ++i)
        {
            const std::uint8_t value = report[i];
            if (!ByteDeviates(i, value))
            {
                continue;
            }
            ++counts_[i * 256 + value];
            if (Distance(value, rest_[i]) > Distance(peak_[i], rest_[i]))
            {
                peak_[i] = value;
            }
        }
    }

    void Calibrator::Claim(const Binding& binding)
    {
        if (binding.kind == BindingKind::Bit)
        {
            claimed_[binding.byteIndex] |= binding.mask;
        }
        else if (binding.kind == BindingKind::Analog)
        {
            claimed_[binding.byteIndex] = 0xFF;
        }
        // Hat não é reservado: as quatro direções compartilham o mesmo byte.
    }

    std::vector<Calibrator::Candidate> Calibrator::Candidates() const
    {
        std::vector<Candidate> result;
        for (std::size_t i = 0; i < rest_.size(); ++i)
        {
            std::uint32_t hits = 0;
            std::uint32_t best = 0;
            std::uint8_t held = rest_[i];
            for (int value = 0; value < 256; ++value)
            {
                const std::uint32_t count = counts_[i * 256 + static_cast<std::size_t>(value)];
                hits += count;
                if (count > best)
                {
                    best = count;
                    held = static_cast<std::uint8_t>(value);
                }
            }
            if (hits > 0)
            {
                result.push_back({ i, rest_[i], held, peak_[i], hits });
            }
        }
        return result;
    }

    std::optional<Binding> Calibrator::Detect(ControlKind kind) const
    {
        const auto candidates = Candidates();
        if (candidates.empty())
        {
            return std::nullopt;
        }

        switch (kind)
        {
        case ControlKind::Axis:
            return DetectAnalog(candidates, false);

        case ControlKind::Trigger:
            if (auto analog = DetectAnalog(candidates, true)) return analog;
            return DetectBit(candidates, false);

        case ControlKind::DPad:
            if (auto hat = DetectHat(candidates)) return hat;
            [[fallthrough]];

        case ControlKind::Button:
        default:
            if (auto bit = DetectBit(candidates, true)) return bit;
            if (auto analog = DetectAnalog(candidates, false)) return analog;
            return DetectBit(candidates, false);
        }
    }

    std::optional<Binding> Calibrator::DetectBit(const std::vector<Candidate>& candidates, bool singleBitOnly) const
    {
        const Candidate* best = nullptr;
        std::uint8_t bestMask = 0;
        for (const Candidate& c : candidates)
        {
            const auto mask = static_cast<std::uint8_t>((c.held ^ c.rest) & ~claimed_[c.index]);
            if (mask == 0 || (singleBitOnly && std::popcount(mask) != 1))
            {
                continue;
            }
            if (best == nullptr || c.hits > best->hits)
            {
                best = &c;
                bestMask = mask;
            }
        }
        if (best == nullptr)
        {
            return std::nullopt;
        }
        Binding binding;
        binding.kind = BindingKind::Bit;
        binding.byteIndex = static_cast<std::uint16_t>(best->index);
        binding.mask = bestMask;
        binding.value = static_cast<std::uint8_t>(best->held & bestMask);
        return binding;
    }

    std::optional<Binding> Calibrator::DetectHat(const std::vector<Candidate>& candidates) const
    {
        // Hat típico: repouso com nibble 8 (ou 15) e direções com nibble 0-7.
        const Candidate* best = nullptr;
        for (const Candidate& c : candidates)
        {
            const int restNibble = c.rest & 0x0F;
            const int heldNibble = c.held & 0x0F;
            const bool singleBitFlip = std::popcount(static_cast<unsigned>(restNibble ^ heldNibble)) == 1;
            if (restNibble >= 8 && heldNibble < 8 && !(singleBitFlip && restNibble != 8))
            {
                if (best == nullptr || c.hits > best->hits)
                {
                    best = &c;
                }
            }
        }
        if (best == nullptr)
        {
            return std::nullopt;
        }
        Binding binding;
        binding.kind = BindingKind::Hat;
        binding.byteIndex = static_cast<std::uint16_t>(best->index);
        binding.mask = 0x0F;
        binding.value = static_cast<std::uint8_t>(best->held & 0x0F);
        return binding;
    }

    std::optional<Binding> Calibrator::DetectAnalog(const std::vector<Candidate>& candidates, bool skipBitLike)
    {
        const Candidate* best = nullptr;
        int bestDistance = kAnalogThreshold - 1;
        for (const Candidate& c : candidates)
        {
            const int distance = Distance(c.peak, c.rest);
            // Um único bit alterado sugere botão, não gatilho analógico.
            if (skipBitLike && std::popcount(static_cast<unsigned>(c.peak ^ c.rest)) == 1)
            {
                continue;
            }
            if (distance > bestDistance)
            {
                bestDistance = distance;
                best = &c;
            }
        }
        if (best == nullptr)
        {
            return std::nullopt;
        }
        Binding binding;
        binding.kind = BindingKind::Analog;
        binding.byteIndex = static_cast<std::uint16_t>(best->index);
        binding.rest = best->rest;
        binding.full = best->peak;
        return binding;
    }

    CalibrationStatus Calibrator::Status() const
    {
        CalibrationStatus status;
        status.phase = phase_;
        status.stepIndex = std::min(step_, kControlCount);
        status.message = message_;

        switch (phase_)
        {
        case CalibrationPhase::CapturingRest:
            status.prompt = L"Solte o controle";
            status.message = L"Não toque em nada enquanto o repouso é medido...";
            break;
        case CalibrationPhase::WaitingRelease:
            status.prompt = L"Solte todos os botões e analógicos";
            break;
        case CalibrationPhase::WaitingInput:
            status.prompt = GetControlInfo(static_cast<ControlId>(step_)).prompt;
            break;
        case CalibrationPhase::Settling:
            status.prompt = GetControlInfo(static_cast<ControlId>(step_)).prompt;
            status.message = L"Capturando... continue segurando.";
            break;
        case CalibrationPhase::Finished:
            status.prompt = L"Calibração concluída";
            break;
        }
        return status;
    }
}
