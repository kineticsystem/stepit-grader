#!/usr/bin/env bash
# Builds the libraries StepIt Grader needs into deps/, from their own source
# releases, each pinned to a version and checked against its SHA-256: LibRaw
# and Little CMS, which raw-converter finds with pkg-config, libtiff, pugixml
# and GoogleTest, which CMake finds. Run it once, from the repository's root,
# on Linux, on macOS, or on Windows in an MSYS2 MINGW64 shell. It needs a C and
# a C++ compiler, CMake, make, curl, and zlib, which every system has.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${ROOT}/deps"
WORK="${ROOT}/build/deps-src"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
mkdir -p "${PREFIX}" "${WORK}"
export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

# Downloads a release once, checks it, and unpacks it into build/deps-src.
fetch() {
    local url="$1" sha256="$2" file="${WORK}/$(basename "$1")"
    [[ -f "${file}" ]] || curl -fL --retry 3 -o "${file}" "${url}"
    if command -v sha256sum > /dev/null; then
        echo "${sha256}  ${file}" | sha256sum --check --status
    else
        echo "${sha256}  ${file}" | shasum -a 256 --check --status
    fi || { echo "${file}: wrong checksum" >&2; exit 1; }
    tar -xzf "${file}" -C "${WORK}"
}

# Builds a CMake project of build/deps-src into deps/, as a static library that
# can go into a shared one, so nothing has to be shipped beside the programs.
cmake_install() {
    local source="$1"
    shift
    cmake -S "${WORK}/${source}" -B "${WORK}/${source}/_build" -Wno-dev \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_PREFIX_PATH="${PREFIX}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        "$@" > /dev/null
    cmake --build "${WORK}/${source}/_build" --parallel "${JOBS}" > /dev/null
    cmake --install "${WORK}/${source}/_build" > /dev/null
}

echo "Little CMS 2.17"
fetch https://github.com/mm2/Little-CMS/releases/download/lcms2.17/lcms2-2.17.tar.gz \
    d11af569e42a1baa1650d20ad61d12e41af4fead4aa7964a01f93b08b53ab074
(cd "${WORK}/lcms2-2.17" &&
    ./configure --prefix="${PREFIX}" --libdir="${PREFIX}/lib" --disable-static --quiet &&
    make -j"${JOBS}" --quiet && make install --quiet)

echo "LibRaw 0.21.4"
fetch https://www.libraw.org/data/LibRaw-0.21.4.tar.gz \
    6be43f19397e43214ff56aab056bf3ff4925ca14012ce5a1538a172406a09e63
# Shared, as its LGPL licence asks of a program that links it. No OpenMP:
# Apple's clang has none, and the editor runs its own threads.
(cd "${WORK}/LibRaw-0.21.4" &&
    ./configure --prefix="${PREFIX}" --libdir="${PREFIX}/lib" --disable-static \
        --disable-examples --disable-openmp --enable-lcms --quiet &&
    make -j"${JOBS}" --quiet && make install --quiet)

echo "libtiff 4.7.0"
fetch https://download.osgeo.org/libtiff/tiff-4.7.0.tar.gz \
    67160e3457365ab96c5b3286a0903aa6e78bdc44c4bc737d2e486bcecb6ba976
# Only Deflate, with the system's zlib, besides what libtiff does alone, e.g.
# LZW: the codecs of other libraries are off, so none is found by chance.
cmake_install tiff-4.7.0 \
    -Djpeg=OFF -Dold-jpeg=OFF -Djbig=OFF -Dlerc=OFF -Dlzma=OFF -Dwebp=OFF -Dzstd=OFF \
    -Dlibdeflate=OFF -Dzlib=ON -Dpixarlog=ON -Dcxx=OFF \
    -Dtiff-tools=OFF -Dtiff-tests=OFF -Dtiff-contrib=OFF -Dtiff-docs=OFF

echo "pugixml 1.15"
fetch https://github.com/zeux/pugixml/releases/download/v1.15/pugixml-1.15.tar.gz \
    655ade57fa703fb421c2eb9a0113b5064bddb145d415dd1f88c79353d90d511a
cmake_install pugixml-1.15 -DPUGIXML_BUILD_TESTS=OFF

echo "GoogleTest 1.15.2"
fetch https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz \
    7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
cmake_install googletest-1.15.2 -DBUILD_GMOCK=OFF -DINSTALL_GTEST=ON -Dgtest_force_shared_crt=ON

echo "Installed into ${PREFIX}"
