#pragma once

#include "core/gamepad_state.h"

#include <memory>
#include <string>

namespace ps3translator
{
    // Gamepad Xbox 360 virtual criado pelo driver ViGEmBus. O Windows o enxerga como
    // um controle XInput nativo.
    class VirtualXbox
    {
    public:
        VirtualXbox();
        ~VirtualXbox();

        VirtualXbox(const VirtualXbox&) = delete;
        VirtualXbox& operator=(const VirtualXbox&) = delete;

        // Conecta ao driver e "pluga" o controle virtual. Falha se o ViGEmBus não estiver instalado.
        [[nodiscard]] bool Connect(std::wstring& error);
        void Disconnect();
        [[nodiscard]] bool IsConnected() const;

        // Envia o estado atual para o controle virtual.
        bool Submit(const GamepadState& state);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
