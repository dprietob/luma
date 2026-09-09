#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Luma Contributors
#
# Compila en Release, pasa la batería de tests y genera el paquete nativo con
# CPack. Está pensado para ejecutarse dentro del contenedor de la distro destino
# (ver packaging/build-packages.sh), pero funciona igual en cualquier entorno que
# tenga las dependencias instaladas.
#
# Uso: build-package.sh <DEB|RPM>
#
# Variables de entorno:
#   LUMA_SRC_DIR    árbol de fuentes        (por defecto: /src)
#   LUMA_OUT_DIR    destino del paquete     (por defecto: /out)
#   LUMA_BUILD_DIR  directorio de compilación (por defecto: /build)

set -euo pipefail

GENERATOR="${1:?falta el generador de CPack (DEB|RPM)}"
SRC_DIR="${LUMA_SRC_DIR:-/src}"
OUT_DIR="${LUMA_OUT_DIR:-/out}"
BUILD_DIR="${LUMA_BUILD_DIR:-/build}"

log() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }

mkdir -p "${OUT_DIR}"

log "Configurando y compilando (Release)…"
cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DLUMA_BUILD_TESTS=ON
cmake --build "${BUILD_DIR}"

log "Ejecutando los tests…"
QT_QPA_PLATFORM=offscreen ctest --test-dir "${BUILD_DIR}" --output-on-failure

log "Generando el paquete ${GENERATOR}…"
cpack --config "${BUILD_DIR}/CPackConfig.cmake" -G "${GENERATOR}" -B "${BUILD_DIR}/dist"

log "Copiando artefactos a ${OUT_DIR}…"
find "${BUILD_DIR}/dist" -maxdepth 1 -type f \( -name '*.deb' -o -name '*.rpm' \) \
    -exec cp -v {} "${OUT_DIR}/" \;

# Con Docker (rootful) los ficheros quedarían como root; se ajustan al dueño del
# directorio de salida para que sean utilizables desde el host.
find "${OUT_DIR}" -maxdepth 1 -type f \( -name '*.deb' -o -name '*.rpm' \) \
    -exec chown --reference="${OUT_DIR}" {} +
