#include "calibration/calibrator.h"
#include "calibration/profile_store.h"
#include "translator/report_translator.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace
{
    using namespace ps3translator;
    using Clock = Calibrator::Clock;
    using namespace std::chrono_literals;

    void Expect(bool condition, const char* description)
    {
        if (!condition)
        {
            std::cerr << "FAILED: " << description << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    // Report fictício de 6 bytes: [0]=ID, [1]=botões, [2]=hat, [3]=LX, [4]=LT, [5]=ruído.
    ReportBytes Rest() { return { 0, 0x00, 0x08, 128, 0, 0 }; }

    // Alimenta o calibrador por 'duration' simulando um report constante.
    void Hold(Calibrator& c, const ReportBytes& report, Clock::time_point& now, std::chrono::milliseconds duration)
    {
        const auto end = now + duration;
        while (now < end)
        {
            c.Feed(report, now);
            now += 10ms;
        }
    }
}

int main()
{
    Calibrator calibrator(0x1234, 0x5678);
    calibrator.Begin();
    auto now = Clock::now();

    Hold(calibrator, Rest(), now, 1700ms);
    Expect(calibrator.Status().phase == CalibrationPhase::WaitingInput, "rest capture finishes");

    // Passo A: bit 0x10 do byte 1.
    ReportBytes pressed = Rest();
    pressed[1] = 0x10;
    Hold(calibrator, pressed, now, 700ms);
    const Binding a = calibrator.Profile().bindings[static_cast<size_t>(ControlId::A)];
    Expect(a.kind == BindingKind::Bit && a.byteIndex == 1 && a.mask == 0x10 && a.value == 0x10, "button bit detected");

    // Pula B, X, Y, LB e RB e depois captura LT analógico.
    Hold(calibrator, Rest(), now, 100ms);
    for (int i = 0; i < 5; ++i) calibrator.Skip();
    Hold(calibrator, Rest(), now, 100ms);

    ReportBytes trigger = Rest();
    trigger[4] = 255;
    Hold(calibrator, trigger, now, 700ms);
    const Binding lt = calibrator.Profile().bindings[static_cast<size_t>(ControlId::LeftTrigger)];
    Expect(lt.kind == BindingKind::Analog && lt.byteIndex == 4 && lt.full == 255, "trigger analog detected");

    // Perfil: tradução e ida e volta pelo arquivo.
    std::stringstream stream;
    WriteProfile(stream, calibrator.Profile());
    const auto loaded = ReadProfile(stream);
    Expect(loaded.has_value() && loaded->vendorId == 0x1234, "profile round trip");

    ReportBytes live = Rest();
    live[1] = 0x10;
    live[4] = 255;
    const GamepadState state = TranslateReport(*loaded, live);
    Expect(state.a && state.leftTrigger > 0.99F && !state.b, "report translation");

    // Hat: direção 0 (cima) também aciona diagonais vizinhas (1 e 7).
    DeviceProfile hatProfile;
    hatProfile.bindings[static_cast<size_t>(ControlId::DpadUp)] = { BindingKind::Hat, 2, 0x0F, 0, 0, 0 };
    ReportBytes hatReport = Rest();
    hatReport[2] = 1;
    Expect(TranslateReport(hatProfile, hatReport).dpadUp, "hat diagonal triggers up");
    hatReport[2] = 8;
    Expect(!TranslateReport(hatProfile, hatReport).dpadUp, "hat released");

    return EXIT_SUCCESS;
}
