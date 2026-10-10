cmake_minimum_required(VERSION 3.20)

# Runtime estático: o exe não exige o VC++ Redistributable instalado.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(PS3Translator LANGUAGES CXX RC)

include(CTest)
include(FetchContent)

# ViGEmClient: cliente do driver ViGEmBus (cria o Xbox 360 virtual).
# SOURCE_SUBDIR inexistente impede o CMake de processar o projeto deles; compilamos só a fonte necessária.
FetchContent_Declare(vigem_client
    GIT_REPOSITORY https://github.com/nefarius/ViGEmClient.git
    GIT_TAG v1.21.222.0
    SOURCE_SUBDIR _none_
)
FetchContent_MakeAvailable(vigem_client)

# Lógica pura (sem Windows): mapeamento, calibração, perfis e tradução de reports.
add_library(PS3Core
    src/core/input_mapping.cpp
    src/calibration/control_map.cpp
    src/calibration/calibrator.cpp
    src/calibration/profile_store.cpp
    src/translator/report_translator.cpp
)
target_include_directories(PS3Core PUBLIC src)
target_compile_features(PS3Core PUBLIC cxx_std_20)

# Acesso a dispositivos HID (enumeração e leitura de reports).
add_library(PS3Hid
    src/hid/hid_device.cpp
    src/hid/hid_reader.cpp
)
target_include_directories(PS3Hid PUBLIC src)
target_compile_features(PS3Hid PUBLIC cxx_std_20)
target_compile_definitions(PS3Hid PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(PS3Hid PUBLIC PS3Core hid setupapi)

# Código de terceiros: ViGEmClient compilado como biblioteca estática.
add_library(ViGEmClient STATIC ${vigem_client_SOURCE_DIR}/src/ViGEmClient.cpp)
target_include_directories(ViGEmClient PUBLIC ${vigem_client_SOURCE_DIR}/include PRIVATE ${vigem_client_SOURCE_DIR}/src)
target_compile_features(ViGEmClient PUBLIC cxx_std_20)
target_link_libraries(ViGEmClient PUBLIC setupapi)

# Xbox virtual.
add_library(PS3Xbox src/xbox/virtual_xbox.cpp)
target_link_libraries(PS3Xbox PUBLIC PS3Core ViGEmClient)
target_compile_definitions(PS3Xbox PUBLIC NOMINMAX)

# Console: lista dispositivos e mostra reports brutos.
add_executable(PS3Translator src/cli/cli_main.cpp)
target_link_libraries(PS3Translator PRIVATE PS3Hid)

# Aplicativo gráfico (Win32) "ys ps3": gera ys-ps3.exe com ícone e versão embutidos.
add_executable(PS3TranslatorApp WIN32
    src/app/gui_main.cpp
    src/app/session.cpp
    src/app/settings.cpp
    src/app/app.rc
    src/ui/main_window.cpp
)
set_target_properties(PS3TranslatorApp PROPERTIES OUTPUT_NAME ys-ps3)
target_link_libraries(PS3TranslatorApp PRIVATE PS3Hid PS3Xbox comctl32 shell32 advapi32)

if(BUILD_TESTING)
    add_executable(PS3CoreTests
        tests/input_mapping_tests.cpp
    )
    target_link_libraries(PS3CoreTests PRIVATE PS3Core)
    add_test(NAME PS3CoreTests COMMAND PS3CoreTests)

    add_executable(CalibrationTests tests/calibration_tests.cpp)
    target_link_libraries(CalibrationTests PRIVATE PS3Core)
    add_test(NAME CalibrationTests COMMAND CalibrationTests)
endif()

if(MSVC)
    foreach(target PS3Core PS3Hid PS3Xbox PS3Translator PS3TranslatorApp)
        target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc /utf-8)
    endforeach()
    # Os cabeçalhos do ViGEm não são UTF-8 válidos.
    target_compile_options(PS3Xbox PRIVATE /wd4828)
    if(BUILD_TESTING)
        target_compile_options(PS3CoreTests PRIVATE /W4 /permissive-)
        target_compile_options(CalibrationTests PRIVATE /W4 /permissive- /utf-8)
    endif()
endif()
