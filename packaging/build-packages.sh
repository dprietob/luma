#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Luma Contributors
#
# Genera los binarios distribuibles de Luma para las distros soportadas.
#
# Cada paquete se compila DENTRO de un contenedor de su distro destino, de modo
# que el enlazado dinámico apunte a las versiones de Qt y de las librerías de
# audio de esa distro. El host solo necesita podman (o docker).
#
# Uso:
#   ./packaging/build-packages.sh              # deb + rpm + appimage
#   ./packaging/build-packages.sh deb rpm      # solo los indicados
#   ENGINE=docker ./packaging/build-packages.sh
#   FEDORA_VERSION=43 ./packaging/build-packages.sh rpm
#   DEBIAN_SUITE=forky ./packaging/build-packages.sh deb
#
# Objetivos:
#   deb       .deb para Debian 13+ / Ubuntu 25.04+  (contenedor debian:trixie)
#   rpm       .rpm para una release de Fedora       (contenedor fedora:44)
#   appimage  AppImage para Ubuntu 22.04+, Debian 12+, Fedora 36+ y Arch
#             (contenedor ubuntu:22.04 con Qt oficial)
#
# Arch Linux se empaqueta desde fuentes con packaging/arch/PKGBUILD; no genera
# un binario distribuible aquí.
#
# Salida: dist/

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
CONTAINERS_DIR="${ROOT_DIR}/packaging/containers"

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
fail() { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

ENGINE="${ENGINE:-}"
if [[ -z "${ENGINE}" ]]; then
    for candidate in podman docker; do
        if command -v "${candidate}" > /dev/null; then ENGINE="${candidate}"; break; fi
    done
fi
[[ -n "${ENGINE}" ]] || fail "no se encontró podman ni docker; instala uno de los dos"

FEDORA_VERSION="${FEDORA_VERSION:-44}"
DEBIAN_SUITE="${DEBIAN_SUITE:-trixie}"

TARGETS=("$@")
[[ ${#TARGETS[@]} -gt 0 ]] || TARGETS=(deb rpm appimage)

mkdir -p "${DIST_DIR}"

run_in_container() {
    local image="$1"; shift
    "${ENGINE}" run --rm \
        -v "${ROOT_DIR}:/src:ro,z" \
        -v "${DIST_DIR}:/out:z" \
        "${image}" "$@"
}

build_image() {
    local target="$1" tag="$2"
    shift 2
    log "Construyendo la imagen ${tag}…"
    "${ENGINE}" build \
        -f "${CONTAINERS_DIR}/Containerfile.${target}" \
        -t "${tag}" \
        "$@" \
        "${CONTAINERS_DIR}"
}

for target in "${TARGETS[@]}"; do
    case "${target}" in
        deb)
            build_image deb "luma-build-deb:${DEBIAN_SUITE}" \
                --build-arg "DEBIAN_SUITE=${DEBIAN_SUITE}"
            log "Generando el .deb (debian:${DEBIAN_SUITE})…"
            run_in_container "luma-build-deb:${DEBIAN_SUITE}" \
                /src/packaging/containers/build-package.sh DEB
            ;;
        rpm)
            build_image rpm "luma-build-rpm:${FEDORA_VERSION}" \
                --build-arg "FEDORA_VERSION=${FEDORA_VERSION}"
            log "Generando el .rpm (fedora:${FEDORA_VERSION})…"
            run_in_container "luma-build-rpm:${FEDORA_VERSION}" \
                /src/packaging/containers/build-package.sh RPM
            ;;
        appimage)
            build_image appimage luma-build-appimage
            log "Generando el AppImage…"
            run_in_container luma-build-appimage \
                /src/packaging/containers/build-appimage-container.sh
            ;;
        *)
            fail "objetivo desconocido: ${target} (deb|rpm|appimage)"
            ;;
    esac
done

log "Artefactos en ${DIST_DIR}:"
ls -lh "${DIST_DIR}"
