; Instalador do ys ps3 (Inno Setup 6). Compile com: ISCC installer\ys-ps3.iss
; Pré-requisito: cmake --build build --config Release

#define MyAppName "ys ps3"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "ysh-rael"
#define MyAppExe "ys-ps3.exe"
#define ViGEmInstaller "ViGEmBus_1.22.0_x64_x86_arm64.exe"

[Setup]
AppId={{B7A4D0E2-5C1F-4E63-9B8A-2F6D3C9E1A47}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
; Texto mostrado na página de aceite (licença + termo de isenção).
LicenseFile=..\LICENSE.txt
SetupIconFile=..\resources\ysps3_64.ico
UninstallDisplayIcon={app}\{#MyAppExe}
OutputDir=..\dist
OutputBaseFilename=ys-ps3-setup-{#MyAppVersion}
; Admin é necessário para instalar o driver ViGEmBus.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Mesmo nome do mutex criado pelo aplicativo: avisa/fecha o app se estiver rodando.
AppMutex=ysps3_single_instance
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Components]
Name: "main"; Description: "ys ps3"; Types: full custom; Flags: fixed
Name: "vigem"; Description: "Driver ViGEmBus (necessário para criar o controle Xbox virtual)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na área de trabalho"; Flags: unchecked
Name: "startup"; Description: "Iniciar com o Windows (em segundo plano)"

[Files]
Source: "..\build\Release\{#MyAppExe}"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\resources\ysps3_64.ico"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\resources\{#ViGEmInstaller}"; DestDir: "{tmp}"; Flags: deleteafterinstall; Components: vigem

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; IconFilename: "{app}\ysps3_64.ico"
Name: "{group}\Desinstalar {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; IconFilename: "{app}\ysps3_64.ico"; Tasks: desktopicon

[Registry]
; Mesma chave usada pela opção "Iniciar com o Windows" do aplicativo.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "ys-ps3"; \
    ValueData: """{app}\{#MyAppExe}"" --background"; Flags: uninsdeletevalue; Tasks: startup

[Run]
Filename: "{tmp}\{#ViGEmInstaller}"; Parameters: "/quiet /norestart"; StatusMsg: "Instalando o driver ViGEmBus..."; \
    Flags: waituntilterminated; Components: vigem
Filename: "{app}\{#MyAppExe}"; Description: "Abrir o ys ps3"; Flags: nowait postinstall skipifsilent
