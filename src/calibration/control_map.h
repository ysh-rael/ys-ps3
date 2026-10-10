#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ps3translator
{
    // Controles do gamepad Xbox 360 que podem ser mapeados.
    // A ordem deste enum é também a ordem em que a calibração pede cada controle.
    enum class ControlId : std::uint8_t
    {
        A, B, X, Y,
        LeftShoulder, RightShoulder,
        LeftTrigger, RightTrigger,
        Back, Start, Guide,
        LeftStick, RightStick,
        DpadUp, DpadDown, DpadLeft, DpadRight,
        LeftStickX, LeftStickY, RightStickX, RightStickY,
        Count
    };

    inline constexpr std::size_t kControlCount = static_cast<std::size_t>(ControlId::Count);

    // Tipo físico esperado do controle; define como a calibração detecta a entrada.
    enum class ControlKind : std::uint8_t
    {
        Button,  // botão digital
        DPad,    // direção do direcional (bit ou hat switch)
        Axis,    // eixo analógico de -1 a 1
        Trigger  // gatilho analógico (ou digital) de 0 a 1
    };

    struct ControlInfo
    {
        const char* key;          // chave estável usada no arquivo de perfil
        const wchar_t* prompt;    // instrução mostrada ao usuário
        ControlKind kind;
    };

    [[nodiscard]] const ControlInfo& GetControlInfo(ControlId id);
}
