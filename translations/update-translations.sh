#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Extrae las cadenas traducibles del código (qsTr/tr) y (re)genera los ficheros
# .po por idioma en translations/. Requiere las herramientas de Qt Linguist
# (paquete qt6-linguist: lupdate, lconvert).
#
#   Uso:  translations/update-translations.sh
#
# El build de CMake compila cada translations/luma_<lang>.po a un .qm
# embebido en :/i18n (ver CMakeLists.txt). La app carga el .qm según el idioma
# elegido en Preferencias o el locale del sistema.

set -euo pipefail

cd "$(dirname "$0")/.."

LANGS=(es fr de it pt nl)
TS_ALL="translations/luma.ts"

# 1) Extrae/actualiza el catálogo maestro desde las fuentes.
lupdate -recursive src -ts "$TS_ALL"

# 2) Genera/actualiza un .po por idioma (conserva las traducciones existentes).
for lang in "${LANGS[@]}"; do
    po="translations/luma_${lang}.po"
    if [ -f "$po" ]; then
        tmp_ts="$(mktemp --suffix=.ts)"
        lconvert "$po" -o "$tmp_ts"
        lconvert "$TS_ALL" "$tmp_ts" -o "$po"
        rm -f "$tmp_ts"
    else
        lconvert "$TS_ALL" -o "$po"
    fi
    echo "updated $po"
done

echo "Done. Edit the .po files and rebuild to compile the .qm."
