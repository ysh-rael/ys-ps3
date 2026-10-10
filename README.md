<img width="1583" height="633" alt="tumb-ys-ps3" src="https://github.com/user-attachments/assets/b6277eed-f781-41f8-aab8-a5626af5b5e9" />

# PS3 Translator

Aplicativo Windows (C++20, Win32 nativo) que lê um controle PS3 genérico, permite calibrá-lo botão a botão e o expõe ao Windows como um **controle Xbox 360 (XInput)**.

## Requisitos

- Windows 10/11 e Visual Studio (Build Tools) com CMake.
- Driver **ViGEmBus** instalado (https://github.com/nefarius/ViGEmBus/releases). É ele que cria o Xbox virtual; sem o driver a ponte não inicia.
- Acesso à internet na primeira configuração do CMake (baixa o código do ViGEmClient).

## Compilação

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Executáveis em `build\Debug\`:

- `PS3TranslatorApp.exe`: aplicativo gráfico.
- `PS3Translator.exe`: ferramenta de console (`--list`, `--watch <índice>`) para inspecionar reports HID brutos.

## Como usar

1. Conecte o controle e abra `PS3TranslatorApp.exe`.
2. Escolha o controle na lista (marque "Mostrar todos" se ele não aparecer).
3. Clique em **Calibrar** e siga as instruções na tela: solte o controle por alguns segundos, depois aperte/segure cada botão, direcional, gatilho e analógico quando pedido. **Pular** ignora o controle atual.
4. Ao final o perfil é salvo automaticamente e não precisa ser refeito.
5. Clique em **Iniciar Xbox virtual**. O Windows passa a ver um controle Xbox 360 (confira em `joy.cpl` ou em qualquer jogo XInput). **Parar** remove o controle virtual.

Perfis ficam em `%APPDATA%\PS3Translator\profiles\<VID>_<PID>.profile` (texto simples). Para recalibrar, basta calibrar de novo; para apagar, remova o arquivo.

Dica: jogos podem ver o controle físico e o virtual ao mesmo tempo (entrada duplicada). Use uma ferramenta como HidHide para ocultar o físico.

## Como funciona

```
HID físico -> HidReader -> Calibrator (modo calibração)
                        -> TranslateReport -> VirtualXbox (modo ponte)
```

A calibração não conhece o layout do controle: ela compara cada report com o estado de repouso.

1. **Repouso**: durante ~1,5 s mede cada byte. Bytes que oscilam (acelerômetro etc.) são ignorados.
2. **Cada controle**: ao detectar mudança, coleta amostras por 0,5 s e classifica o resultado:
   - `Bit`: um bit mudou; guarda byte, máscara e valor (funciona com lógica positiva ou negativa).
   - `Hat`: direcional em hat switch (nibble 0-7, sentido horário; diagonais acionam as duas direções).
   - `Analog`: byte que varia entre repouso e extremo (analógicos e gatilhos).
3. Bits/bytes já atribuídos são reservados para não serem capturados duas vezes.

Limitação: eixos são lidos com 8 bits (1 byte). Controles com eixos de 16 bits não são suportados.

## Segundo plano

O app (ys-ps3.exe) fica na bandeja do sistema: fechar a janela apenas a esconde; use "Sair" no menu do ícone para encerrar. Com "Ativar automaticamente" marcado, a cada 2 s ele procura um controle já calibrado e liga o Xbox virtual; se o controle for desconectado, o Xbox virtual é removido e volta quando ele for reconectado. "Iniciar com o Windows" abre o app com `--background` (somente na bandeja). Parar o Xbox virtual manualmente desliga o modo automático.

## Instalador

Compile em Release e gere o instalador com o Inno Setup 6:

```powershell
cmake --build build --config Release
ISCC installer\ys-ps3.iss   # saída em dist\
```

O instalador exibe a licença e o termo de isenção de [LICENSE.txt](LICENSE.txt), e oferece o driver ViGEmBus (`resources\`) como componente opcional, marcado por padrão.

## Estrutura

| Pasta | Conteúdo |
| --- | --- |
| `src/core` | `GamepadState`, normalização de eixos/gatilhos, tipo `ReportBytes` |
| `src/calibration` | Lista de controles Xbox, `Calibrator` (máquina de estados), perfil e persistência |
| `src/translator` | Converte um report bruto em `GamepadState` usando o perfil |
| `src/hid` | Enumeração de dispositivos HID e leitura de reports em thread própria |
| `src/xbox` | `VirtualXbox`: envia o estado ao driver ViGEmBus |
| `src/app` | `Session` (coordena modos Idle/Calibrando/Ponte) e ponto de entrada da GUI |
| `src/ui` | Janela Win32 (combo de dispositivos, instruções, progresso, botões, DPI e redimensionamento) |
| `src/cli` | Ferramenta de console |
| `tests` | Testes de mapeamento, calibração, perfil e tradução |

`PS3Core` não depende de Windows, o que permite testar a calibração com reports simulados.
