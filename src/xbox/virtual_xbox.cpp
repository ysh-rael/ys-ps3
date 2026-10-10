#include "xbox/virtual_xbox.h"

#include <windows.h>
#include <ViGEm/Client.h>

#include <algorithm>
#include <cstdint>

namespace ps3translator
{
    struct VirtualXbox::Impl
    {
        PVIGEM_CLIENT client = nullptr;
        PVIGEM_TARGET target = nullptr;
    };

    namespace
    {
        std::int16_t ToThumb(float value)
        {
            return static_cast<std::int16_t>(std::clamp(value, -1.0F, 1.0F) * 32767.0F);
        }

        std::uint8_t ToTrigger(float value)
        {
            return static_cast<std::uint8_t>(std::clamp(value, 0.0F, 1.0F) * 255.0F);
        }

        XUSB_REPORT ToReport(const GamepadState& s)
        {
            XUSB_REPORT report{};
            const auto set = [&report](bool pressed, USHORT flag) { if (pressed) report.wButtons |= flag; };
            set(s.a, XUSB_GAMEPAD_A);
            set(s.b, XUSB_GAMEPAD_B);
            set(s.x, XUSB_GAMEPAD_X);
            set(s.y, XUSB_GAMEPAD_Y);
            set(s.dpadUp, XUSB_GAMEPAD_DPAD_UP);
            set(s.dpadDown, XUSB_GAMEPAD_DPAD_DOWN);
            set(s.dpadLeft, XUSB_GAMEPAD_DPAD_LEFT);
            set(s.dpadRight, XUSB_GAMEPAD_DPAD_RIGHT);
            set(s.leftShoulder, XUSB_GAMEPAD_LEFT_SHOULDER);
            set(s.rightShoulder, XUSB_GAMEPAD_RIGHT_SHOULDER);
            set(s.start, XUSB_GAMEPAD_START);
            set(s.back, XUSB_GAMEPAD_BACK);
            set(s.guide, XUSB_GAMEPAD_GUIDE);
            set(s.leftStick, XUSB_GAMEPAD_LEFT_THUMB);
            set(s.rightStick, XUSB_GAMEPAD_RIGHT_THUMB);
            report.bLeftTrigger = ToTrigger(s.leftTrigger);
            report.bRightTrigger = ToTrigger(s.rightTrigger);
            report.sThumbLX = ToThumb(s.leftX);
            report.sThumbLY = ToThumb(s.leftY);
            report.sThumbRX = ToThumb(s.rightX);
            report.sThumbRY = ToThumb(s.rightY);
            return report;
        }
    }

    VirtualXbox::VirtualXbox() : impl_(std::make_unique<Impl>()) {}

    VirtualXbox::~VirtualXbox() { Disconnect(); }

    bool VirtualXbox::IsConnected() const { return impl_->target != nullptr; }

    bool VirtualXbox::Connect(std::wstring& error)
    {
        Disconnect();

        impl_->client = vigem_alloc();
        if (impl_->client == nullptr)
        {
            error = L"Falha ao inicializar o cliente ViGEm.";
            return false;
        }

        const VIGEM_ERROR connectResult = vigem_connect(impl_->client);
        if (!VIGEM_SUCCESS(connectResult))
        {
            error = connectResult == VIGEM_ERROR_BUS_NOT_FOUND
                ? L"Driver ViGEmBus não encontrado. Instale-o em github.com/nefarius/ViGEmBus/releases e tente de novo."
                : L"Não foi possível conectar ao driver ViGEmBus.";
            Disconnect();
            return false;
        }

        impl_->target = vigem_target_x360_alloc();
        if (impl_->target == nullptr || !VIGEM_SUCCESS(vigem_target_add(impl_->client, impl_->target)))
        {
            error = L"Não foi possível criar o controle Xbox virtual.";
            Disconnect();
            return false;
        }
        return true;
    }

    void VirtualXbox::Disconnect()
    {
        if (impl_->client != nullptr)
        {
            if (impl_->target != nullptr)
            {
                vigem_target_remove(impl_->client, impl_->target);
                vigem_target_free(impl_->target);
                impl_->target = nullptr;
            }
            vigem_disconnect(impl_->client);
            vigem_free(impl_->client);
            impl_->client = nullptr;
        }
        else if (impl_->target != nullptr)
        {
            vigem_target_free(impl_->target);
            impl_->target = nullptr;
        }
    }

    bool VirtualXbox::Submit(const GamepadState& state)
    {
        if (!IsConnected())
        {
            return false;
        }
        return VIGEM_SUCCESS(vigem_target_x360_update(impl_->client, impl_->target, ToReport(state)));
    }
}
