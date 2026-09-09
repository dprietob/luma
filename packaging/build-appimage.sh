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
# Variables de entorno (para reutilizar el script dentro de un contenedor con el
# árbol de fuentes montado en solo lectura; ver packaging/build-packages.sh):
#   LUMA_BUILD_DIR   directorio de compilación   (por defecto: build-appimage/)
#   LUMA_TOOLS_DIR   caché de linuxdeploy et al. (por defecto: packaging/tools/)
#   LUMA_OUTPUT_DIR  destino del AppImage        (por defecto: raíz del repo)
#   LUMA_RESTORE_HOST_ELF  restaurar los ELF originales tras patchelf (ver §5b;
#                          por defecto 1, necesario en distros con RELR)
#
# Requisitos: las mismas -devel de compilación (ver README) + FUSE para
# ejecutar AppImages (o exportar APPIMAGE_EXTRACT_AND_RUN=1 en contenedores/CI).

set -euo pipefail

ARCH="${ARCH:-$(uname -m)}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${LUMA_BUILD_DIR:-${ROOT_DIR}/build-appimage}"
APPDIR="${BUILD_DIR}/AppDir"
TOOLS_DIR="${LUMA_TOOLS_DIR:-${ROOT_DIR}/packaging/tools}"

log() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }

QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake)}"
[[ -x "${QMAKE}" ]] || { echo "error: no se encontró qmake6 ni qmake en el PATH" >&2; exit 1; }
export QMAKE

# --- 1. Compilación en Release ---
log "Configurando y compilando (Release)…"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLUMA_BUILD_TESTS=OFF
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
restore_host_elfs() {
    local qt_plugins qt_qml ldconfig_cache line
    qt_plugins="$("${QMAKE}" -query QT_INSTALL_PLUGINS)"
    qt_qml="$("${QMAKE}" -query QT_INSTALL_QML)"

    # Librerías compartidas: se localizan por soname en la caché de ldconfig (se
    # lee una sola vez; evita un pipe por iteración que dispararía SIGPIPE bajo
    # `set -o pipefail`).
    ldconfig_cache="$(ldconfig -p)"
    while IFS= read -r lib; do
        line="$(grep -m1 -F "$(basename "$lib") (" <<<"${ldconfig_cache}" || true)"
        if [[ -n "${line}" ]]; then restore_from_host "$lib" "${line##*=> }"; fi
    done < <(find "${APPDIR}/usr/lib" -maxdepth 1 -name '*.so*' -type f)

    # Plugins de Qt y módulos QML: misma jerarquía relativa que el Qt del host.
    while IFS= read -r f; do
        restore_from_host "$f" "${qt_plugins}/${f#"${APPDIR}"/usr/plugins/}"
    done < <(find "${APPDIR}/usr/plugins" -name '*.so' -type f)
    while IFS= read -r f; do
        restore_from_host "$f" "${qt_qml}/${f#"${APPDIR}"/usr/qml/}"
    done < <(find "${APPDIR}/usr/qml" -name '*.so' -type f)
}
restore_from_host() { if [[ -e "$2" ]]; then cp -L --remove-destination "$2" "$1"; fi; }

if [[ "${LUMA_RESTORE_HOST_ELF:-1}" == "1" ]]; then
    log "Restaurando ELF originales del host (evita corrupción RELR de patchelf)…"
    restore_host_elfs
fi

# --- 6. Generación del AppImage con appimagetool (no reañade dependencias) ---
log "Generando el AppImage…"
VERSION="$(sed -nE 's/^ *VERSION ([0-9.]+) *$/\1/p' "${ROOT_DIR}/CMakeLists.txt" | head -1)"
export OUTPUT="${LUMA_OUTPUT_DIR:-${ROOT_DIR}}/Luma-${VERSION}-${ARCH}.AppImage"
"${TOOLS_DIR}/appimagetool-${ARCH}.AppImage" "${APPDIR}" "${OUTPUT}"

log "Listo: ${OUTPUT}"
