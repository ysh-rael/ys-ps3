#include "calibration/control_map.h"

namespace ps3translator
{
    namespace
    {
        // Mesma ordem do enum ControlId.
        constexpr std::array<ControlInfo, kControlCount> kControls{ {
            { "A", L"Pressione o botão A (X / Cruz no PS3)", ControlKind::Button },
            { "B", L"Pressione o botão B (Círculo no PS3)", ControlKind::Button },
            { "X", L"Pressione o botão X (Quadrado no PS3)", ControlKind::Button },
            { "Y", L"Pressione o botão Y (Triângulo no PS3)", ControlKind::Button },
            { "LB", L"Pressione LB (L1 no PS3)", ControlKind::Button },
            { "RB", L"Pressione RB (R1 no PS3)", ControlKind::Button },
            { "LT", L"Aperte e segure o gatilho LT (L2 no PS3)", ControlKind::Trigger },
            { "RT", L"Aperte e segure o gatilho RT (R2 no PS3)", ControlKind::Trigger },
            { "Back", L"Pressione Back (Select no PS3)", ControlKind::Button },
            { "Start", L"Pressione Start", ControlKind::Button },
            { "Guide", L"Pressione o botão central (PS / Guide)", ControlKind::Button },
            { "LS", L"Pressione o analógico esquerdo (L3)", ControlKind::Button },
            { "RS", L"Pressione o analógico direito (R3)", ControlKind::Button },
            { "DpadUp", L"Pressione e segure o direcional para CIMA", ControlKind::DPad },
            { "DpadDown", L"Pressione e segure o direcional para BAIXO", ControlKind::DPad },
            { "DpadLeft", L"Pressione e segure o direcional para a ESQUERDA", ControlKind::DPad },
            { "DpadRight", L"Pressione e segure o direcional para a DIREITA", ControlKind::DPad },
            { "LX", L"Mova o analógico esquerdo todo para a DIREITA e segure", ControlKind::Axis },
            { "LY", L"Mova o analógico esquerdo todo para CIMA e segure", ControlKind::Axis },
            { "RX", L"Mova o analógico direito todo para a DIREITA e segure", ControlKind::Axis },
            { "RY", L"Mova o analógico direito todo para CIMA e segure", ControlKind::Axis },
        } };
    }

    const ControlInfo& GetControlInfo(ControlId id)
    {
        return kControls[static_cast<std::size_t>(id)];
    }
}
