# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Luma Contributors

find_package(Qt6 6.4 REQUIRED COMPONENTS Test)

function(add_mixer_test name)
    qt_add_executable(${name} ${name}.cpp MockAudioBackend.h)
    set_target_properties(${name} PROPERTIES AUTOMOC ON)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${name} PRIVATE mixer_engine Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_mixer_test(tst_audiochannel)
add_mixer_test(tst_importflow)

# Test de integración del fader: arrastra el slider real y comprueba que el
# volumen del AudioChannel baja (reproduce el fallo reportado en la UI).
find_package(Qt6 6.4 REQUIRED COMPONENTS QuickTest Quick QuickControls2)
qt_add_executable(tst_fader_integration tst_fader_integration.cpp MockAudioBackend.h)
set_target_properties(tst_fader_integration PROPERTIES AUTOMOC ON)
target_include_directories(tst_fader_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tst_fader_integration PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qmltests")
target_link_libraries(tst_fader_integration PRIVATE
    mixer_engine Qt6::QuickTest Qt6::Quick Qt6::QuickControls2 Qt6::Qml Qt6::Gui)
add_test(NAME tst_fader_integration COMMAND tst_fader_integration)
set_tests_properties(tst_fader_integration PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_decoder y tst_mixer generan fixtures WAV con libsndfile.
add_mixer_test(tst_decoder)
target_link_libraries(tst_decoder PRIVATE PkgConfig::SNDFILE)
target_compile_definitions(tst_decoder PRIVATE FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

add_mixer_test(tst_mixer)
target_link_libraries(tst_mixer PRIVATE PkgConfig::SNDFILE)

# Persistencia de sesión JSON (Fase 4): ida y vuelta + casos límite.
add_mixer_test(tst_session)

# Presets/escenas: captura, restauración independiente y detención al cambiar.
add_mixer_test(tst_preset)

# Flujos (Fase 0): CRUD, round-trip JSON y persistencia en sesión.
add_mixer_test(tst_flow)

# Proyectos (.luma): creación de carpeta, copia de pistas, rutas relativas y recientes.
add_mixer_test(tst_project)
