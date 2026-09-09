# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Luma Contributors

# Configuración de CPack para generar los paquetes nativos de las distros
# soportadas (.deb para Debian/Ubuntu, .rpm para Fedora). Arch se empaqueta
# desde fuentes con el PKGBUILD de packaging/arch/.
#
# Los paquetes se generan en un contenedor de la distro destino, de modo que el
# enlazado dinámico apunte a las librerías de esa distro; ver
# packaging/build-packages.sh.

set(CPACK_PACKAGE_NAME luma)
set(CPACK_PACKAGE_VENDOR "Luma Contributors")
set(CPACK_PACKAGE_CONTACT "Luma Contributors <https://github.com/dprietob/luma/issues>")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/dprietob/luma")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Real-time multitrack audio mixer")
set(CPACK_PACKAGE_DESCRIPTION
    "Luma plays many audio tracks at once and lets you control each one \
independently, with the look and feel of a professional studio tool: per-channel \
volume, pan, looping, timed fades, effects, equalizer, presets and node-graph \
playback flows.")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_STRIP_FILES ON)
set(CPACK_SOURCE_GENERATOR "")

# --- Debian / Ubuntu ---
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION sound)
set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
set(CPACK_DEBIAN_PACKAGE_RELEASE 1)

# shlibdeps resuelve las librerías enlazadas (Qt, PortAudio, sndfile, mpg123,
# SoundTouch). Los módulos QML, el plugin de plataforma y el plugin de imagen SVG
# (los iconos de la UI son SVG) se cargan en tiempo de ejecución, no se enlazan,
# así que shlibdeps no los ve y hay que declararlos.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "qt6-qpa-plugins, \
qt6-svg-plugins, \
qml6-module-qtqml-workerscript, \
qml6-module-qtqml-models, \
qml6-module-qtquick-controls, \
qml6-module-qtquick-templates, \
qml6-module-qtquick-dialogs, \
qml6-module-qtquick-layouts, \
qml6-module-qtquick-window")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "pipewire-pulse | pulseaudio")

# --- Fedora ---
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_URL ${CPACK_PACKAGE_HOMEPAGE_URL})
set(CPACK_RPM_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtdeclarative, qt6-qtbase-gui, qt6-qtsvg")
set(CPACK_RPM_PACKAGE_SUGGESTS "pipewire-pulseaudio")

# Los directorios estándar del escritorio pertenecen a `filesystem`,
# `hicolor-icon-theme` y `shared-mime-info`; reclamarlos provocaría conflictos.
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications
    ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons
    ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons/hicolor
    ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/metainfo
    ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/mime
    ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/mime/packages)
foreach(icon_size 64 128 256 512)
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons/hicolor/${icon_size}x${icon_size}
        ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons/hicolor/${icon_size}x${icon_size}/apps)
endforeach()

include(CPack)
