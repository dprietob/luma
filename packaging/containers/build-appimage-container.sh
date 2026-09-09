#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Luma Contributors
#
# Envoltorio de packaging/build-appimage.sh para ejecutarlo dentro del
# contenedor de AppImage (ver packaging/build-packages.sh): redirige el
# directorio de compilación, la caché de herramientas y la salida fuera del árbol
# de fuentes, que está montado en solo lectura.
#
# Montajes esperados:
#   /src  → árbol de fuentes (solo lectura)
#   /out  → directorio de salida en el host

set -euo pipefail

# Los contenedores no tienen FUSE, así que las herramientas AppImage se
# autoextraen en lugar de montarse.
export APPIMAGE_EXTRACT_AND_RUN=1

# patchelf funciona correctamente con el toolchain de la imagen (sin RELR), así
# que no hace falta restaurar los ELF originales.
export LUMA_RESTORE_HOST_ELF=0

export LUMA_BUILD_DIR=/build
export LUMA_TOOLS_DIR=/opt/appimage-tools
export LUMA_OUTPUT_DIR=/out

/src/packaging/build-appimage.sh

find /out -maxdepth 1 -type f -name '*.AppImage' -exec chown --reference=/out {} +
