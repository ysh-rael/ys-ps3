#pragma once

#include "app/session.h"
#include "hid/hid_device.h"

#include <windows.h>

#include <vector>

namespace ps3translator::ui
{
    // Janela principal (Win32 puro): escolher dispositivo, calibrar e ativar o Xbox virtual.
    class MainWindow
    {
    public:
        // Mensagem registrada que pede à instância já aberta para mostrar a janela.
        static UINT ShowRequestMessage();

        // Cria a janela e executa o loop de mensagens. startHidden inicia só na bandeja.
        static int Run(HINSTANCE instance, int showCommand, bool startHidden);

    private:
        MainWindow() = default;

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

        void OnCreate();
        void OnCommand(int id);
        void CreateFonts();
        void Layout();            // reposiciona os controles conforme o tamanho/DPI
        void RefreshDevices();    // reenumera os dispositivos HID e preenche o combo
        void UpdateUi();          // sincroniza textos/botões com o estado da sessão
        void AutoConnect();       // em segundo plano: ativa o Xbox virtual para um controle calibrado
        void AddTrayIcon();
        void RemoveTrayIcon();
        void ShowFromTray();
        void ShowTrayMenu();

        [[nodiscard]] const hid::DeviceInfo* SelectedDevice() const;
        [[nodiscard]] int Scale(int value) const;

        HWND hwnd_ = nullptr;
        HWND deviceLabel_ = nullptr;
        HWND deviceCombo_ = nullptr;
        HWND refreshButton_ = nullptr;
        HWND showAllCheck_ = nullptr;
        HWND autoCheck_ = nullptr;
        HWND startupCheck_ = nullptr;
        HWND profileLabel_ = nullptr;
        HWND promptLabel_ = nullptr;
        HWND messageLabel_ = nullptr;
        HWND progress_ = nullptr;
        HWND calibrateButton_ = nullptr;
        HWND skipButton_ = nullptr;
        HWND bridgeButton_ = nullptr;

        HFONT normalFont_ = nullptr;
        HFONT titleFont_ = nullptr;
        UINT dpi_ = 96;
        bool quitting_ = false;  // false: fechar a janela apenas a esconde na bandeja

        std::vector<hid::DeviceInfo> devices_;  // lista exibida no combo (mesma ordem)
        Session session_;
    };
}
