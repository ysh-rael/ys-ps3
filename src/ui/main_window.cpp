#include "ui/main_window.h"

#include "calibration/profile_store.h"
#include "app/app_resources.h"
#include "app/settings.h"

#include <commctrl.h>
#include <shellapi.h>

#include <algorithm>
#include <string>
#include <utility>

namespace ps3translator::ui
{
    namespace
    {
        constexpr wchar_t kClassName[] = L"YsPs3MainWindow";
        constexpr wchar_t kAppTitle[] = L"ys ps3";

        enum ControlIds
        {
            IdDeviceCombo = 100,
            IdRefresh,
            IdShowAll,
            IdCalibrate,
            IdSkip,
            IdBridge,
            IdAuto,
            IdStartup,
            IdTrayOpen,
            IdTrayExit
        };

        constexpr UINT_PTR kPollTimer = 1;
        constexpr UINT_PTR kAutoTimer = 2;
        constexpr UINT kPollIntervalMs = 50;
        constexpr UINT kAutoIntervalMs = 2000;       // frequência da busca por controles calibrados
        constexpr UINT kMsgDeviceLost = WM_APP + 1;  // enviado pela thread de leitura
        constexpr UINT kMsgTray = WM_APP + 2;        // eventos do ícone da bandeja
        constexpr UINT kTrayId = 1;

        HICON LoadAppIcon(int size)
        {
            return static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
                                                 size, size, LR_DEFAULTCOLOR));
        }

        HWND MakeChild(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id)
        {
            return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
        }
    }

    UINT MainWindow::ShowRequestMessage()
    {
        static const UINT message = RegisterWindowMessageW(L"ys-ps3.ShowWindow");
        return message;
    }

    int MainWindow::Run(HINSTANCE instance, int showCommand, bool startHidden)
    {
        INITCOMMONCONTROLSEX controls{ sizeof(controls), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&controls);

        MainWindow window;

        WNDCLASSEXW wc{ sizeof(wc) };
        wc.lpfnWndProc = &MainWindow::WindowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadAppIcon(GetSystemMetrics(SM_CXICON));
        wc.hIconSm = LoadAppIcon(GetSystemMetrics(SM_CXSMICON));
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);

        // 'window' é passado como lpParam e associado ao HWND em WM_NCCREATE.
        HWND hwnd = CreateWindowExW(0, kClassName, kAppTitle, WS_OVERLAPPEDWINDOW,
                                    CW_USEDEFAULT, CW_USEDEFAULT, 620, 480, nullptr, nullptr, instance, &window);
        if (hwnd == nullptr)
        {
            return 1;
        }

        if (!startHidden)
        {
            ShowWindow(hwnd, showCommand);
            UpdateWindow(hwnd);
        }

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

    LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = nullptr;
        if (message == WM_NCCREATE)
        {
            self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            self->hwnd_ = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        return self != nullptr ? self->HandleMessage(message, wParam, lParam) : DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT MainWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            OnCreate();
            return 0;

        case WM_SIZE:
            Layout();
            return 0;

        case WM_GETMINMAXINFO:
        {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize = { Scale(560), Scale(460) };
            return 0;
        }

        case WM_DPICHANGED:
        {
            dpi_ = HIWORD(wParam);
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                         suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            CreateFonts();
            Layout();
            return 0;
        }

        case WM_COMMAND:
            OnCommand(LOWORD(wParam));
            return 0;

        case WM_TIMER:
            if (wParam == kPollTimer)
            {
                UpdateUi();
            }
            else if (wParam == kAutoTimer)
            {
                AutoConnect();
            }
            return 0;

        case WM_CLOSE:
            // Fechar apenas esconde: o app continua na bandeja. "Sair" no menu da bandeja encerra de verdade.
            if (quitting_)
            {
                DestroyWindow(hwnd_);
            }
            else
            {
                ShowWindow(hwnd_, SW_HIDE);
            }
            return 0;

        case kMsgTray:
            if (LOWORD(lParam) == WM_LBUTTONDBLCLK || LOWORD(lParam) == WM_LBUTTONUP)
            {
                ShowFromTray();
            }
            else if (LOWORD(lParam) == WM_RBUTTONUP)
            {
                ShowTrayMenu();
            }
            return 0;

        case kMsgDeviceLost:
            session_.Stop(L"O controle foi desconectado.");
            RefreshDevices();
            UpdateUi();
            return 0;

        case WM_CTLCOLORSTATIC:
            // Rótulos com o mesmo fundo da janela.
            SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));

        case WM_DESTROY:
            KillTimer(hwnd_, kPollTimer);
            KillTimer(hwnd_, kAutoTimer);
            RemoveTrayIcon();
            session_.Stop();
            if (normalFont_) DeleteObject(normalFont_);
            if (titleFont_) DeleteObject(titleFont_);
            PostQuitMessage(0);
            return 0;

        default:
            if (message == ShowRequestMessage())
            {
                ShowFromTray();
                return 0;
            }
            return DefWindowProcW(hwnd_, message, wParam, lParam);
        }
    }

    void MainWindow::OnCreate()
    {
        dpi_ = GetDpiForWindow(hwnd_);

        deviceLabel_ = MakeChild(hwnd_, L"STATIC", L"Controle:", SS_LEFT, 0);
        deviceCombo_ = MakeChild(hwnd_, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, IdDeviceCombo);
        refreshButton_ = MakeChild(hwnd_, L"BUTTON", L"Atualizar", BS_PUSHBUTTON | WS_TABSTOP, IdRefresh);
        showAllCheck_ = MakeChild(hwnd_, L"BUTTON", L"Mostrar todos os dispositivos HID", BS_AUTOCHECKBOX | WS_TABSTOP, IdShowAll);
        profileLabel_ = MakeChild(hwnd_, L"STATIC", L"", SS_LEFT, 0);
        autoCheck_ = MakeChild(hwnd_, L"BUTTON", L"Ativar automaticamente em segundo plano quando um controle calibrado for conectado",
                               BS_AUTOCHECKBOX | WS_TABSTOP, IdAuto);
        startupCheck_ = MakeChild(hwnd_, L"BUTTON", L"Iniciar com o Windows (minimizado na bandeja)",
                                  BS_AUTOCHECKBOX | WS_TABSTOP, IdStartup);
        SendMessageW(autoCheck_, BM_SETCHECK, settings::GetAutoBridge() ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(startupCheck_, BM_SETCHECK, settings::GetRunAtStartup() ? BST_CHECKED : BST_UNCHECKED, 0);
        promptLabel_ = MakeChild(hwnd_, L"STATIC", L"", SS_CENTER, 0);
        messageLabel_ = MakeChild(hwnd_, L"STATIC", L"", SS_CENTER, 0);
        progress_ = MakeChild(hwnd_, PROGRESS_CLASSW, L"", PBS_SMOOTH, 0);
        calibrateButton_ = MakeChild(hwnd_, L"BUTTON", L"Calibrar", BS_PUSHBUTTON | WS_TABSTOP, IdCalibrate);
        skipButton_ = MakeChild(hwnd_, L"BUTTON", L"Pular", BS_PUSHBUTTON | WS_TABSTOP, IdSkip);
        bridgeButton_ = MakeChild(hwnd_, L"BUTTON", L"Iniciar Xbox virtual", BS_PUSHBUTTON | WS_TABSTOP, IdBridge);

        CreateFonts();
        Layout();
        RefreshDevices();
        UpdateUi();

        session_.onDeviceLost = [hwnd = hwnd_] { PostMessageW(hwnd, kMsgDeviceLost, 0, 0); };
        SetTimer(hwnd_, kPollTimer, kPollIntervalMs, nullptr);
        SetTimer(hwnd_, kAutoTimer, kAutoIntervalMs, nullptr);
        AddTrayIcon();
    }

    void MainWindow::AddTrayIcon()
    {
        NOTIFYICONDATAW data{ sizeof(data) };
        data.hWnd = hwnd_;
        data.uID = kTrayId;
        data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        data.uCallbackMessage = kMsgTray;
        data.hIcon = LoadAppIcon(GetSystemMetrics(SM_CXSMICON));
        wcscpy_s(data.szTip, kAppTitle);
        Shell_NotifyIconW(NIM_ADD, &data);
    }

    void MainWindow::RemoveTrayIcon()
    {
        NOTIFYICONDATAW data{ sizeof(data) };
        data.hWnd = hwnd_;
        data.uID = kTrayId;
        Shell_NotifyIconW(NIM_DELETE, &data);
    }

    void MainWindow::ShowFromTray()
    {
        ShowWindow(hwnd_, SW_SHOW);
        ShowWindow(hwnd_, SW_RESTORE);
        SetForegroundWindow(hwnd_);
    }

    void MainWindow::ShowTrayMenu()
    {
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, IdTrayOpen, L"Abrir ys ps3");
        AppendMenuW(menu, MF_STRING, IdTrayExit, L"Sair");
        POINT cursor{};
        GetCursorPos(&cursor);
        SetForegroundWindow(hwnd_);  // necessário para o menu fechar ao clicar fora
        TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd_, nullptr);
        DestroyMenu(menu);
    }

    void MainWindow::AutoConnect()
    {
        if (SendMessageW(autoCheck_, BM_GETCHECK, 0, 0) != BST_CHECKED || session_.Poll().mode != SessionMode::Idle)
        {
            return;
        }

        for (const hid::DeviceInfo& device : hid::EnumerateHidDevices())
        {
            if (!device.canOpen || device.caps.InputReportByteLength == 0 || !hid::IsGameController(device)
                || !ProfileExists(device.attributes.VendorID, device.attributes.ProductID))
            {
                continue;
            }
            if (session_.StartBridge(device))
            {
                RefreshDevices();
                for (size_t i = 0; i < devices_.size(); ++i)
                {
                    if (devices_[i].path == device.path)
                    {
                        SendMessageW(deviceCombo_, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
                    }
                }
                UpdateUi();
            }
            return;
        }
    }

    void MainWindow::OnCommand(int id)
    {
        switch (id)
        {
        case IdRefresh:
        case IdShowAll:
            RefreshDevices();
            break;

        case IdDeviceCombo:
            break;

        case IdAuto:
            settings::SetAutoBridge(SendMessageW(autoCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED);
            break;

        case IdStartup:
            settings::SetRunAtStartup(SendMessageW(startupCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED);
            break;

        case IdTrayOpen:
            ShowFromTray();
            break;

        case IdTrayExit:
            quitting_ = true;
            PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            break;

        case IdCalibrate:
        {
            const SessionStatus status = session_.Poll();
            if (status.mode == SessionMode::Calibrating)
            {
                session_.Stop(L"Calibração cancelada.");
            }
            else if (const hid::DeviceInfo* device = SelectedDevice())
            {
                (void)session_.StartCalibration(*device);
            }
            break;
        }

        case IdSkip:
            session_.SkipStep();
            break;

        case IdBridge:
        {
            const SessionStatus status = session_.Poll();
            if (status.mode == SessionMode::Bridging)
            {
                // Parada manual: desliga o modo automático para não reativar logo em seguida.
                SendMessageW(autoCheck_, BM_SETCHECK, BST_UNCHECKED, 0);
                settings::SetAutoBridge(false);
                session_.Stop(L"Xbox virtual desativado.");
            }
            else if (const hid::DeviceInfo* device = SelectedDevice())
            {
                (void)session_.StartBridge(*device);
            }
            break;
        }
        }
        UpdateUi();
    }

    void MainWindow::CreateFonts()
    {
        if (normalFont_) DeleteObject(normalFont_);
        if (titleFont_) DeleteObject(titleFont_);

        NONCLIENTMETRICSW metrics{ sizeof(metrics) };
        SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi_);
        normalFont_ = CreateFontIndirectW(&metrics.lfMessageFont);

        LOGFONTW title = metrics.lfMessageFont;
        title.lfHeight = static_cast<LONG>(title.lfHeight * 1.6);
        title.lfWeight = FW_SEMIBOLD;
        titleFont_ = CreateFontIndirectW(&title);

        for (HWND control : { deviceLabel_, deviceCombo_, refreshButton_, showAllCheck_, autoCheck_, startupCheck_, profileLabel_, messageLabel_,
                              calibrateButton_, skipButton_, bridgeButton_ })
        {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont_), TRUE);
        }
        SendMessageW(promptLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont_), TRUE);
    }

    int MainWindow::Scale(int value) const
    {
        return MulDiv(value, static_cast<int>(dpi_), 96);
    }

    void MainWindow::Layout()
    {
        if (deviceLabel_ == nullptr)
        {
            return;
        }
        RECT client{};
        GetClientRect(hwnd_, &client);

        const int margin = Scale(16);
        const int gap = Scale(8);
        const int rowHeight = Scale(26);
        const int buttonHeight = Scale(36);
        const int width = client.right - 2 * margin;
        int y = margin;

        MoveWindow(deviceLabel_, margin, y, width, Scale(20), TRUE);
        y += Scale(22);

        const int refreshWidth = Scale(100);
        // Altura do combo inclui a lista suspensa.
        MoveWindow(deviceCombo_, margin, y, width - refreshWidth - gap, Scale(300), TRUE);
        MoveWindow(refreshButton_, margin + width - refreshWidth, y, refreshWidth, rowHeight, TRUE);
        y += rowHeight + gap;

        MoveWindow(showAllCheck_, margin, y, width, rowHeight, TRUE);
        y += rowHeight;
        MoveWindow(autoCheck_, margin, y, width, rowHeight, TRUE);
        y += rowHeight;
        MoveWindow(startupCheck_, margin, y, width, rowHeight, TRUE);
        y += rowHeight;
        MoveWindow(profileLabel_, margin, y, width, rowHeight, TRUE);
        y += rowHeight + gap;

        // Área de instruções ocupa o espaço restante acima da barra e dos botões.
        const int bottomArea = Scale(24) + gap + buttonHeight + margin;
        const int promptAreaHeight = std::max(Scale(80), static_cast<int>(client.bottom) - y - bottomArea - Scale(40));
        MoveWindow(promptLabel_, margin, y, width, promptAreaHeight, TRUE);
        y += promptAreaHeight;
        MoveWindow(messageLabel_, margin, y, width, Scale(40), TRUE);

        const int progressY = client.bottom - margin - buttonHeight - gap - Scale(16);
        MoveWindow(progress_, margin, progressY, width, Scale(16), TRUE);

        const int buttonWidth = (width - 2 * gap) / 3;
        const int buttonY = client.bottom - margin - buttonHeight;
        MoveWindow(calibrateButton_, margin, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(skipButton_, margin + buttonWidth + gap, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(bridgeButton_, margin + 2 * (buttonWidth + gap), buttonY, buttonWidth, buttonHeight, TRUE);
    }

    void MainWindow::RefreshDevices()
    {
        // Preserva a seleção atual (por caminho) ao reenumerar.
        std::wstring previousPath;
        if (const hid::DeviceInfo* current = SelectedDevice())
        {
            previousPath = current->path;
        }

        const bool showAll = SendMessageW(showAllCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        devices_.clear();
        SendMessageW(deviceCombo_, CB_RESETCONTENT, 0, 0);
        int selected = 0;
        for (auto& device : hid::EnumerateHidDevices())
        {
            if (!device.canOpen || device.caps.InputReportByteLength == 0 || (!showAll && !hid::IsGameController(device)))
            {
                continue;
            }
            if (device.path == previousPath)
            {
                selected = static_cast<int>(devices_.size());
            }
            SendMessageW(deviceCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(hid::DisplayName(device).c_str()));
            devices_.push_back(std::move(device));
        }
        if (!devices_.empty())
        {
            SendMessageW(deviceCombo_, CB_SETCURSEL, selected, 0);
        }
    }

    const hid::DeviceInfo* MainWindow::SelectedDevice() const
    {
        const auto index = static_cast<size_t>(SendMessageW(deviceCombo_, CB_GETCURSEL, 0, 0));
        return index < devices_.size() ? &devices_[index] : nullptr;
    }

    void MainWindow::UpdateUi()
    {
        const SessionStatus status = session_.Poll();
        const hid::DeviceInfo* device = SelectedDevice();
        const bool idle = status.mode == SessionMode::Idle;
        const bool hasProfile = device != nullptr
            && ProfileExists(device->attributes.VendorID, device->attributes.ProductID);

        // Rótulo do perfil do dispositivo selecionado.
        SetWindowTextW(profileLabel_, device == nullptr ? L"Nenhum controle encontrado. Conecte-o e clique em Atualizar."
                                      : hasProfile      ? L"Perfil: calibrado (salvo)"
                                                        : L"Perfil: ainda não calibrado");

        std::wstring prompt;
        std::wstring message = status.message;
        int progress = 0;
        switch (status.mode)
        {
        case SessionMode::Calibrating:
            prompt = status.calibration.prompt;
            message = status.calibration.message;
            progress = static_cast<int>(status.calibration.stepIndex * 100 / status.calibration.stepCount);
            break;
        case SessionMode::Bridging:
            prompt = L"Xbox virtual ativo";
            break;
        default:
            prompt = hasProfile ? L"Pronto para iniciar" : L"Calibre o controle para começar";
            break;
        }

        // Evita piscar: só atualiza texto quando muda.
        const auto setText = [](HWND control, const std::wstring& text)
        {
            wchar_t current[512]{};
            GetWindowTextW(control, current, 512);
            if (text != current)
            {
                SetWindowTextW(control, text.c_str());
            }
        };
        setText(promptLabel_, prompt);
        setText(messageLabel_, message);
        SendMessageW(progress_, PBM_SETPOS, progress, 0);

        setText(calibrateButton_, status.mode == SessionMode::Calibrating ? L"Cancelar calibração" : L"Calibrar");
        setText(bridgeButton_, status.mode == SessionMode::Bridging ? L"Parar Xbox virtual" : L"Iniciar Xbox virtual");

        EnableWindow(deviceCombo_, idle);
        EnableWindow(refreshButton_, idle);
        EnableWindow(showAllCheck_, idle);
        EnableWindow(calibrateButton_, status.mode == SessionMode::Calibrating || (idle && device != nullptr));
        EnableWindow(skipButton_, status.mode == SessionMode::Calibrating);
        EnableWindow(bridgeButton_, status.mode == SessionMode::Bridging || (idle && hasProfile));
    }
}
