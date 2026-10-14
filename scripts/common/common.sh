#!/usr/bin/env bash
# What the scripts of install-deps.sh share. Sourced, not run.

# The operating system: linux, mac or windows.
os_type() {
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        echo "windows"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "mac"
    else
        echo "linux"
    fi
}

# On Windows, MSYS2's tools first, and its own temporary folder: GCC fails with
# "Cannot create temporary file in C:\WINDOWS\" with the Windows one.
if [[ "$(os_type)" == "windows" ]]; then
    [[ -d /c/msys64/mingw64/bin ]] && export PATH="/c/msys64/mingw64/bin:$PATH"
    [[ -d /c/msys64/usr/bin ]] && export PATH="/c/msys64/usr/bin:$PATH"
    export TMPDIR=/tmp TEMP=/tmp TMP=/tmp
fi

# On macOS, every library for both Apple Silicon and Intel, so that the editor
# can be built as one universal program, for the oldest macOS Qt supports. CMake
# and clang read the variables; configure takes the flags.
MACOS_ARCHITECTURES="arm64;x86_64"
export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-13.3}"
if [[ "$(os_type)" == "mac" ]]; then
    export CMAKE_OSX_ARCHITECTURES="$MACOS_ARCHITECTURES"
    ARCH_FLAGS="-arch arm64 -arch x86_64 -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET"
fi

# Adds a folder to a list of folders, e.g. PKG_CONFIG_PATH, once, first.
add_path() {
    local variable="$1" folder="$2"
    local current="${!variable:-}"
    if [[ ":$current:" != *":$folder:"* ]]; then
        export "$variable"="$folder${current:+:$current}"
    fi
}

# Whether a library is installed already: its folder in deps/ exists. Remove
# the folder to build it again.
installed() {
    if [[ -d "$DEPS_DIR/$1" ]]; then
        echo "$1: installed already"
        return 0
    fi
    return 1
}

# Downloads a source release once, into build/deps, checks its SHA-256, and
# unpacks it into a clean folder, whose path it prints.
fetch() {
    local url="$1" sha256="$2" archive="$DOWNLOAD_DIR/$3"
    [[ -f "$archive" ]] || curl -fL --retry 3 -o "$archive" "$url" >&2
    # macOS has shasum, and a sha256sum of its own with other options.
    local actual
    if [[ "$(os_type)" == "mac" ]]; then
        actual="$(shasum -a 256 "$archive")"
    else
        actual="$(sha256sum "$archive")"
    fi
    [[ "${actual%% *}" == "$sha256" ]] || { echo "$archive: wrong checksum" >&2; exit 1; }
    local source="$BUILD_DIR/$(basename "$archive" .tar.gz)"
    rm -rf "$source"
    mkdir -p "$source"
    tar -xzf "$archive" -C "$source" --strip-components=1
    echo "$source"
}

# Builds a CMake project into deps/<name>, as a static library that can go into
# a shared one, so nothing has to be shipped beside the programs.
cmake_install() {
    local source="$1" name="$2"
    shift 2
    local generator=()
    command -v ninja > /dev/null && generator=(-G Ninja)
    cmake -S "$source" -B "$source/_build" "${generator[@]}" -Wno-dev \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_DIR/$name" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        "$@" > /dev/null
    cmake --build "$source/_build" --parallel "$JOBS" > /dev/null
    cmake --install "$source/_build" > /dev/null
}

# Builds an Autotools project into deps/<name>, as a shared library. On macOS
# for both architectures at once, which needs no dependency tracking.
configure_install() {
    local source="$1" name="$2"
    shift 2
    local flags=()
    if [[ -n "${ARCH_FLAGS:-}" ]]; then
        flags=(CFLAGS="$ARCH_FLAGS -O2" CXXFLAGS="$ARCH_FLAGS -O2" LDFLAGS="$ARCH_FLAGS"
            --disable-dependency-tracking)
    fi
    (cd "$source" &&
        sh ./configure --prefix="$DEPS_DIR/$name" --libdir="$DEPS_DIR/$name/lib" \
            --disable-static --quiet "${flags[@]}" "$@" > /dev/null &&
        make -j"$JOBS" --quiet > /dev/null 2>&1 &&
        make install --quiet > /dev/null)
}
