#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Luma Contributors
#
# Empaqueta Luma como AppImage (PROJECT.md §12, Fase 4).
#
# Estrategia: compilar en Release, instalar en un AppDir con la jerarquía FHS
# (usr/bin, usr/share/...) y desplegar las dependencias con linuxdeploy y su
# plugin de Qt, que copia las librerías de Qt, los plugins QML y el runtime
# necesario. Las herramientas se descargan a packaging/tools/ si no existen.
#
# Uso:
#   ./packaging/build-appimage.sh            # build + AppImage
#   ARCH=x86_64 ./packaging/build-appimage.sh
#
# Requisitos: las mismas -devel de compilación (ver README) + FUSE para
# ejecutar AppImages (o exportar APPIMAGE_EXTRACT_AND_RUN=1 en contenedores/CI).

set -euo pipefail

ARCH="${ARCH:-$(uname -m)}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-appimage"
APPDIR="${BUILD_DIR}/AppDir"
TOOLS_DIR="${ROOT_DIR}/packaging/tools"

log() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }

# --- 1. Compilación en Release ---
log "Configurando y compilando (Release)…"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}"

# --- 2. Instalación en el AppDir ---
log "Instalando en ${APPDIR}…"
rm -rf "${APPDIR}"
DESTDIR="${APPDIR}" cmake --install "${BUILD_DIR}" --prefix /usr

# --- 3. Herramientas de despliegue (linuxdeploy + plugin Qt) ---
mkdir -p "${TOOLS_DIR}"
fetch_tool() {
    local name="$1" url="$2"
    if [[ ! -x "${TOOLS_DIR}/${name}" ]]; then
        log "Descargando ${name}…"
        curl -fL --retry 3 -o "${TOOLS_DIR}/${name}" "${url}"
        chmod +x "${TOOLS_DIR}/${name}"
    fi
}
BASE="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous"
QTBASE="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous"
AITOOL="https://github.com/AppImage/appimagetool/releases/download/continuous"
fetch_tool "linuxdeploy-${ARCH}.AppImage"           "${BASE}/linuxdeploy-${ARCH}.AppImage"
fetch_tool "linuxdeploy-plugin-qt-${ARCH}.AppImage" "${QTBASE}/linuxdeploy-plugin-qt-${ARCH}.AppImage"
fetch_tool "appimagetool-${ARCH}.AppImage"          "${AITOOL}/appimagetool-${ARCH}.AppImage"
export PATH="${TOOLS_DIR}:${PATH}"

# --- 4. Despliegue de dependencias (sin empaquetar todavía) ---
log "Desplegando dependencias de Qt en el AppDir…"
export QML_SOURCES_PATHS="${ROOT_DIR}/src/qml"   # para que el plugin Qt escanee los imports QML
# Por defecto el plugin Qt solo empaqueta el plugin de plataforma "xcb". Añadir
# "offscreen" permite ejecutar el AppImage sin display (CI, SSH, headless), como
# el resto del proyecto (README: QT_QPA_PLATFORM=offscreen).
export EXTRA_PLATFORM_PLUGINS="libqoffscreen.so"
# El `strip` que empaqueta linuxdeploy (binutils antiguo) no reconoce la sección
# ELF `.relr.dyn` que genera el toolchain de distros recientes (p. ej. Fedora 44)
# y aborta. NO_STRIP=1 omite ese paso; el AppImage funciona igual, solo pesa algo más.
export NO_STRIP=1
"${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage" \
    --appdir "${APPDIR}" \
    --plugin qt \
    --desktop-file "${APPDIR}/usr/share/applications/io.github.dprietob.luma.desktop" \
    --icon-file "${APPDIR}/usr/share/icons/hicolor/256x256/apps/io.github.dprietob.luma.png"

# --- 5. Limpieza de librerías acopladas al host ---
# libudev/libsystemd van ligadas al udev/systemd del sistema: mejor no
# empaquetarlas y usar las del host (la excludelist oficial de AppImage hace lo
# mismo). El plugin Qt las reañade en su fase de despliegue, así que se borran
# DESPUÉS del despliegue y se empaqueta aparte con appimagetool.
log "Eliminando librerías acopladas al host (libudev, libsystemd)…"
rm -f "${APPDIR}/usr/lib/"libudev.so* "${APPDIR}/usr/lib/"libsystemd.so*

# --- 5b. Reparación de la corrupción de patchelf (RELR) ---
# El patchelf que empaqueta linuxdeploy es antiguo y no entiende la sección ELF
# `.relr.dyn` (relocaciones RELR) que generan las libs de distros recientes
# (Fedora 44). Al reescribir el rpath corrompe esa sección y los punteros de los
# constructores quedan inválidos → SIGSEGV en call_init antes de main(). Se
# sobrescribe cada ELF desplegado con su copia ORIGINAL del host; AppRun ya
# exporta LD_LIBRARY_PATH/QT_PLUGIN_PATH/QML import paths, por lo que el rpath
# que patchelf intentaba fijar no es necesario.
log "Restaurando ELF originales del host (evita corrupción RELR de patchelf)…"
QT_HOST_PLUGINS="$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo /usr/lib64/qt6/plugins)"
QT_HOST_QML="$(qmake6 -query QT_INSTALL_QML 2>/dev/null || echo /usr/lib64/qt6/qml)"
restore_from_host() { if [[ -e "$2" ]]; then cp -L --remove-destination "$2" "$1"; fi; }

# Librerías compartidas: se localizan por soname en la caché de ldconfig (se
# lee una sola vez; evita un pipe por iteración que dispararía SIGPIPE bajo
# `set -o pipefail`).
LDCONFIG_CACHE="$(ldconfig -p)"
while IFS= read -r lib; do
    line="$(grep -m1 -F "$(basename "$lib") (" <<<"${LDCONFIG_CACHE}" || true)"
    if [[ -n "${line}" ]]; then restore_from_host "$lib" "${line##*=> }"; fi
done < <(find "${APPDIR}/usr/lib" -maxdepth 1 -name '*.so*' -type f)

# Plugins de Qt y módulos QML: misma jerarquía relativa que el Qt del host.
while IFS= read -r f; do restore_from_host "$f" "${QT_HOST_PLUGINS}/${f#"${APPDIR}"/usr/plugins/}"; done \
    < <(find "${APPDIR}/usr/plugins" -name '*.so' -type f)
while IFS= read -r f; do restore_from_host "$f" "${QT_HOST_QML}/${f#"${APPDIR}"/usr/qml/}"; done \
    < <(find "${APPDIR}/usr/qml" -name '*.so' -type f)

# --- 6. Generación del AppImage con appimagetool (no reañade dependencias) ---
log "Generando el AppImage…"
export OUTPUT="${ROOT_DIR}/Luma-${ARCH}.AppImage"
"${TOOLS_DIR}/appimagetool-${ARCH}.AppImage" "${APPDIR}" "${OUTPUT}"

log "Listo: ${OUTPUT}"
