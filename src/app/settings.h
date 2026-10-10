#pragma once

namespace ps3translator::settings
{
    // Preferências do usuário, guardadas em HKCU\Software\ysh-rael\ys-ps3.

    // Ativa o Xbox virtual sozinho quando um controle calibrado é conectado (padrão: ligado).
    [[nodiscard]] bool GetAutoBridge();
    void SetAutoBridge(bool enabled);

    // Inicia o aplicativo minimizado na bandeja ao entrar no Windows.
    [[nodiscard]] bool GetRunAtStartup();
    void SetRunAtStartup(bool enabled);
}
