#!/usr/bin/env bash
# Builds the libraries the editor needs that Lentigram's lib/ does not hold,
# LibRaw and Little CMS, into deps/: raw-converter finds them there with
# pkg-config. Run it once, from the repository's root, on Linux, macOS, or
# Windows in an MSYS2 MINGW64 shell.
set -euo pipefail

LIBRAW_VERSION=0.21.4
LCMS2_VERSION=2.17

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${ROOT}/deps"
WORK="${ROOT}/build/deps-src"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
mkdir -p "${PREFIX}" "${WORK}"

fetch() {
    local url="$1" file="${WORK}/$(basename "$1")"
    [[ -f "${file}" ]] || curl -fL --retry 3 -o "${file}" "${url}"
    tar -xzf "${file}" -C "${WORK}"
}

echo "Little CMS ${LCMS2_VERSION}"
fetch "https://github.com/mm2/Little-CMS/releases/download/lcms${LCMS2_VERSION}/lcms2-${LCMS2_VERSION}.tar.gz"
(cd "${WORK}/lcms2-${LCMS2_VERSION}" &&
    ./configure --prefix="${PREFIX}" --disable-static --quiet &&
    make -j"${JOBS}" --quiet && make install --quiet)

echo "LibRaw ${LIBRAW_VERSION}"
fetch "https://www.libraw.org/data/LibRaw-${LIBRAW_VERSION}.tar.gz"
# No OpenMP: Apple's clang has none, and the editor runs its own threads.
(cd "${WORK}/LibRaw-${LIBRAW_VERSION}" &&
    ./configure --prefix="${PREFIX}" --disable-static --disable-examples --disable-openmp \
        --enable-lcms PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig" --quiet &&
    make -j"${JOBS}" --quiet && make install --quiet)

echo "Installed into ${PREFIX}"
