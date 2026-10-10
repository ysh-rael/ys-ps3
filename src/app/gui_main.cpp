#include "ui/main_window.h"

#include <windows.h>

#include <string_view>

// Ativa os controles visuais modernos (Common Controls v6).
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCommand)
{
    // O mesmo nome é usado pelo instalador (AppMutex) para detectar o app em execução.
    HANDLE singleInstance = CreateMutexW(nullptr, TRUE, L"ysps3_single_instance");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Já existe uma instância (provavelmente na bandeja): pede para ela mostrar a janela.
        PostMessageW(HWND_BROADCAST, ps3translator::ui::MainWindow::ShowRequestMessage(), 0, 0);
        return 0;
    }

    // Necessário para a interface ficar nítida em telas com escala (DPI) alta.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const bool startHidden = std::wstring_view(commandLine).find(L"--background") != std::wstring_view::npos;
    const int result = ps3translator::ui::MainWindow::Run(instance, showCommand, startHidden);
    CloseHandle(singleInstance);
    return result;
}
