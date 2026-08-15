# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Luma Contributors

cmake_minimum_required(VERSION 3.21)

project(Luma
    VERSION 1.0.0
    DESCRIPTION "Mezclador de audio multipista para Linux"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# --- Dependencias ---
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.4)

# Los .qml del módulo viven en varios subdirectorios (qml/, components/,
# style/). QTP0004=NEW hace que qt_add_qml_module genere un qmldir por
# subdirectorio, de modo que los tipos sean resolubles entre sí.
qt_policy(SET QTP0004 NEW)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO  REQUIRED IMPORTED_TARGET GLOBAL portaudio-2.0)
pkg_check_modules(SNDFILE    REQUIRED IMPORTED_TARGET GLOBAL sndfile)
pkg_check_modules(MPG123     REQUIRED IMPORTED_TARGET GLOBAL libmpg123)
pkg_check_modules(SOUNDTOUCH REQUIRED IMPORTED_TARGET GLOBAL soundtouch)

# --- Biblioteca del motor/negocio (reutilizada por app y tests) ---
add_library(mixer_engine STATIC
    src/engine/interfaces/IAudioBackend.cpp
    src/engine/AppSettings.cpp
    src/engine/AudioDeviceManager.cpp
    src/engine/AudioEngine.cpp
    src/engine/AudioChannel.cpp
    src/engine/ChannelEffects.cpp
    src/engine/TrackTimeline.cpp
    src/engine/TrackLibrary.cpp
    src/engine/MasterBus.cpp
    src/engine/PresetManager.cpp
    src/engine/ProjectManager.cpp
    src/engine/SessionManager.cpp
    src/engine/flows/FlowManager.cpp
    src/engine/flows/FlowNodeConfig.cpp
    src/engine/flows/FlowPlayer.cpp
    src/engine/backend/AudioFileDecoder.cpp
    src/engine/backend/PortAudioBackend.cpp
    src/engine/dsp/Reverb.cpp
    src/engine/dsp/Delay.cpp
    src/engine/dsp/Distortion.cpp
    src/engine/dsp/Eq.cpp
    src/engine/dsp/VoiceEffects.cpp
)
set_target_properties(mixer_engine PROPERTIES AUTOMOC ON)
target_include_directories(mixer_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/engine)
target_link_libraries(mixer_engine
    PUBLIC  Qt6::Core Qt6::Gui Qt6::Qml
    PRIVATE PkgConfig::PORTAUDIO PkgConfig::SNDFILE PkgConfig::MPG123 PkgConfig::SOUNDTOUCH
)
target_compile_options(mixer_engine PRIVATE -Wall -Wextra -Wpedantic)

# --- Aplicación ---
qt_add_executable(luma src/main.cpp)
target_link_libraries(luma PRIVATE
    mixer_engine Qt6::Gui Qt6::Quick Qt6::QuickControls2)

# Theme.qml y FadeModes.qml son singletons QML (pragma Singleton). Deben
# marcarse ANTES de qt_add_qml_module para que el qmldir los declare como tales.
set_source_files_properties(src/qml/style/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(src/qml/style/FadeModes.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(luma
    URI Luma
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/components/FlowList.qml
        src/qml/components/MixerView.qml
        src/qml/components/WelcomeScreen.qml
        src/qml/components/SplashScreen.qml
        src/qml/components/TrackList.qml
        src/qml/components/WindowButtons.qml
        src/qml/components/atomic/CButton.qml
        src/qml/components/atomic/CMenu.qml
        src/qml/components/atomic/CMenuItem.qml
        src/qml/components/atomic/CTextField.qml
        src/qml/components/atomic/ConfirmDialog.qml
        src/qml/components/atomic/HFader.qml
        src/qml/components/atomic/HSeparator.qml
        src/qml/components/atomic/HSlider.qml
        src/qml/components/atomic/HVuMeter.qml
        src/qml/components/atomic/PanDial.qml
        src/qml/components/atomic/VFader.qml
        src/qml/components/atomic/VSeparator.qml
        src/qml/components/atomic/VSlider.qml
        src/qml/components/atomic/VVuMeter.qml
        src/qml/components/channels/ChannelIndicator.qml
        src/qml/components/channels/ChannelProgress.qml
        src/qml/components/channels/ChannelsArea.qml
        src/qml/components/channels/ChannelStrip.qml
        src/qml/components/channels/EffectButtons.qml
        src/qml/components/channels/EqDialog.qml
        src/qml/components/channels/FadeButtons.qml
        src/qml/components/channels/FxDialog.qml
        src/qml/components/channels/ProgressDialog.qml
        src/qml/components/channels/TransportButtons.qml
        src/qml/components/flows/FlowEdgeConfigDialog.qml
        src/qml/components/flows/FlowEditorDialog.qml
        src/qml/components/flows/FlowNode.qml
        src/qml/components/flows/FlowNodeConfigDialog.qml
        src/qml/components/options/AudioOutputDialog.qml
        src/qml/components/options/ChannelCountDialog.qml
        src/qml/components/options/OptionsArea.qml
        src/qml/components/options/PreferencesDialog.qml
        src/qml/components/presets/PresetButton.qml
        src/qml/components/presets/PresetDialog.qml
        src/qml/components/presets/PresetsArea.qml
        src/qml/components/toolbar/PanicButton.qml
        src/qml/components/toolbar/Toolbar.qml
        src/qml/style/Theme.qml
        src/qml/style/FadeModes.qml
    RESOURCE_PREFIX /qt/qml
)

# Recursos de imagen embebidos (iconos usados desde QML). Se compilan en el
# binario y se referencian con URL qrc estable (qrc:/icons/...), independiente
# del layout de ficheros, de modo que también funcionan en la app distribuida.
qt_add_resources(luma "app_images"
    PREFIX "/icons"
    BASE   "assets/icons"
    FILES
        "assets/icons/thumb.svg"
        "assets/icons/thumb_h.svg"
        "assets/icons/devices.svg"
        "assets/icons/effects.svg"
        "assets/icons/equalizer.svg"
        "assets/icons/fadein.svg"
        "assets/icons/fadeout.svg"
        "assets/icons/grid.svg"
        "assets/icons/left.svg"
        "assets/icons/luma_128.png"
        "assets/icons/menu.svg"
        "assets/icons/number.svg"
        "assets/icons/pause.svg"
        "assets/icons/play.svg"
        "assets/icons/reorder.svg"
        "assets/icons/repeat.svg"
        "assets/icons/reset.svg"
        "assets/icons/right.svg"
        "assets/icons/settings.svg"
        "assets/icons/stop.svg"
        "assets/icons/minimize.svg"
        "assets/icons/maximize.svg"
        "assets/icons/restore.svg"
        "assets/icons/close.svg"
)

# --- Traducciones (.po → .qm, embebidas en :/i18n) ---
# Fuente de traducción: ficheros gettext .po en translations/ (aptos para Flathub/
# Weblate). Se compilan a .qm con las herramientas de Qt Linguist si están
# disponibles (paquete qt6-linguist). Sin ellas, la app funciona en inglés.
find_program(LUMA_LCONVERT NAMES lconvert-qt6 lconvert HINTS /usr/lib64/qt6/bin /usr/bin)
find_program(LUMA_LRELEASE NAMES lrelease-qt6 lrelease HINTS /usr/lib64/qt6/bin /usr/bin)
if(LUMA_LCONVERT AND LUMA_LRELEASE)
    set(LUMA_LANGUAGES es fr de it pt nl)
    set(LUMA_QM_FILES "")
    foreach(lang IN LISTS LUMA_LANGUAGES)
        set(po_file ${CMAKE_CURRENT_SOURCE_DIR}/translations/luma_${lang}.po)
        if(EXISTS ${po_file})
            set(ts_file ${CMAKE_CURRENT_BINARY_DIR}/luma_${lang}.ts)
            set(qm_file ${CMAKE_CURRENT_BINARY_DIR}/luma_${lang}.qm)
            add_custom_command(
                OUTPUT ${qm_file}
                COMMAND ${LUMA_LCONVERT} ${po_file} -o ${ts_file}
                COMMAND ${LUMA_LRELEASE} ${ts_file} -qm ${qm_file}
                DEPENDS ${po_file}
                COMMENT "Compiling translation ${lang}"
                VERBATIM)
            list(APPEND LUMA_QM_FILES ${qm_file})
        endif()
    endforeach()
    if(LUMA_QM_FILES)
        qt_add_resources(luma "translations"
            PREFIX "/i18n"
            BASE   ${CMAKE_CURRENT_BINARY_DIR}
            FILES  ${LUMA_QM_FILES})
    endif()
endif()

# --- Instalación (jerarquía FHS; base del empaquetado AppImage, Fase 4) ---
include(GNUInstallDirs)

install(TARGETS luma
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Entrada de escritorio, icono y metadatos AppStream nombrados con el App ID
# (requisito de Flathub). El basename del .desktop debe coincidir con el App ID.
install(FILES packaging/io.github.dprietob.luma.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
foreach(icon_size 64 128 256 512)
    install(FILES assets/icons/luma_${icon_size}.png
        RENAME io.github.dprietob.luma.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${icon_size}x${icon_size}/apps)
endforeach()
install(FILES packaging/io.github.dprietob.luma.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

# Tipo MIME del proyecto (.luma) para asociar el fichero a la aplicación.
install(FILES packaging/io.github.dprietob.luma.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

# --- Tests ---
option(LUMA_BUILD_TESTS "Build the unit tests" ON)
if(LUMA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
