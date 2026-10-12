#!/usr/bin/env bash
# Builds the libraries StepIt Grader needs, each from its own source release, at
# a fixed version checked against its SHA-256, into a folder of its own in
# deps/, where the build finds them. A library whose folder exists is skipped:
# remove the folder to build it again.
#
# Run it once, from anywhere, on Linux, on macOS, or on Windows in an MSYS2
# MINGW64 shell. It needs a C and a C++ compiler, CMake, make, curl and zlib,
# which every system has; Ninja, when there is one, builds faster.
#
#   scripts/common/common.sh            what the scripts share
#   scripts/<library>/install-<library>.sh  one library: its version, its
#                                       download and how it is built
#
# The order matters: LibRaw needs Little CMS.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export DEPS_DIR="$ROOT/deps"
export DOWNLOAD_DIR="$ROOT/build/deps"
export BUILD_DIR="$ROOT/build/deps"
export JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
mkdir -p "$DEPS_DIR" "$BUILD_DIR"

source "$ROOT/scripts/common/common.sh"

source "$ROOT/scripts/lcms2/install-lcms2.sh"
source "$ROOT/scripts/libraw/install-libraw.sh"
source "$ROOT/scripts/tiff/install-tiff.sh"
source "$ROOT/scripts/pugixml/install-pugixml.sh"
source "$ROOT/scripts/gtest/install-gtest.sh"
source "$ROOT/scripts/linuxdeploy/install-linuxdeploy.sh"

echo "The libraries are in $DEPS_DIR"
