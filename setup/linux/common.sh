#!/usr/bin/env bash
# What the Linux packaging scripts share. Sourced, not run.
#
# Needs Qt6_DIR, e.g. ~/Qt/6.11.0/gcc_64/lib/cmake/Qt6, and the libraries of
# install-deps.sh in deps/.

error() { echo "ERROR: $*" >&2; }

COMMON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PROJECT_ROOT="$(cd "${COMMON_DIR}/../.." && pwd)"
# A build of its own, installed into /usr of an AppDir, apart from the
# development build.
export BUILD_DIR="${PROJECT_ROOT}/build/linux-package"
# The AppImage tools run without FUSE, extracting themselves.
export APPIMAGE_EXTRACT_AND_RUN=1

# The version of the project, from its CMakeLists.txt.
get_version() {
    sed -n 's/^ *VERSION \([0-9.]*\).*/\1/p' "${PROJECT_ROOT}/CMakeLists.txt" | head -1
}

# Builds the project and installs it into ${BUILD_DIR}/AppDir. The tests are
# built too, to be shipped in the AppImage and run there, with its libraries.
build_project() {
    if [[ -z "${Qt6_DIR:-}" ]]; then
        error "Qt6_DIR is not set, e.g. export Qt6_DIR=~/Qt/6.11.0/gcc_64/lib/cmake/Qt6"
        exit 1
    fi
    echo "=== Building into ${BUILD_DIR}"
    cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DSTEPIT_GRADER_BUILD_TESTS=ON > /dev/null
    cmake --build "${BUILD_DIR}" --parallel
    rm -rf "${BUILD_DIR}/AppDir"
    DESTDIR="${BUILD_DIR}/AppDir" cmake --install "${BUILD_DIR}" > /dev/null
}

# linuxdeploy and its Qt plugin, which install-deps.sh downloads into
# deps/linuxdeploy, and qmake, which tells the plugin where Qt is.
setup_linuxdeploy() {
    local tools="${PROJECT_ROOT}/deps/linuxdeploy"
    if [[ ! -x "${tools}/linuxdeploy-x86_64.AppImage" ]]; then
        error "linuxdeploy is not in ${tools}: run ./install-deps.sh"
        exit 1
    fi
    # linuxdeploy finds its plugins on the PATH.
    export PATH="${tools}:${PATH}"
    export QMAKE="${Qt6_DIR%%/lib/cmake*}/bin/qmake"
    if [[ ! -x "${QMAKE}" ]]; then
        error "Cannot find qmake at ${QMAKE}: check Qt6_DIR"
        exit 1
    fi
}

# Makes the AppDir of an application, with linuxdeploy: its programs, Qt, and
# every library they need, from deps/ or from the system.
#   $1  the application, the name of its program, desktop file and icon
#   $2  the other programs to include, space separated: installed ones, or
#       ones of the build only, as the tests are
deploy_appdir() {
    local app="$1" others="${2:-}"
    local appdir="${BUILD_DIR}/AppDir_${app}"
    local installed="${BUILD_DIR}/AppDir/usr"
    rm -rf "${appdir}"
    mkdir -p "${appdir}/usr/bin"
    cp "${installed}/bin/${app}" "${appdir}/usr/bin/"
    local other executables=()
    for other in ${others}; do
        if [[ -f "${installed}/bin/${other}" ]]; then
            cp "${installed}/bin/${other}" "${appdir}/usr/bin/"
        else
            cp "${BUILD_DIR}/bin/${other}" "${appdir}/usr/bin/"
        fi
        executables+=(--executable "${appdir}/usr/bin/${other}")
    done

    # The shared libraries of deps/, e.g. LibRaw, for linuxdeploy to find, and
    # libtiff.so.5, for Qt's TIFF plugin.
    local libs
    libs="$(printf '%s:' "${PROJECT_ROOT}"/deps/*/lib)"
    export LD_LIBRARY_PATH="${libs}${Qt6_DIR%%/lib/cmake*}/lib:${LD_LIBRARY_PATH:-}"

    # The GTK3 theme, for the desktop's fonts and its file dialogs.
    local plugins="${Qt6_DIR%%/lib/cmake*}/plugins"
    if [[ -f "${plugins}/platformthemes/libqgtk3.so" ]]; then
        mkdir -p "${appdir}/usr/plugins/platformthemes"
        cp "${plugins}/platformthemes/libqgtk3.so" "${appdir}/usr/plugins/platformthemes/"
    fi

    # Besides X11, Qt's offscreen display, to run the editor headless, e.g. in tests.
    export EXTRA_PLATFORM_PLUGINS="libqoffscreen.so"
    echo "=== Deploying ${app}"
    linuxdeploy-x86_64.AppImage \
        --appdir "${appdir}" \
        --executable "${appdir}/usr/bin/${app}" \
        "${executables[@]}" \
        --desktop-file "${installed}/share/applications/${app}.desktop" \
        --icon-file "${installed}/share/icons/hicolor/256x256/apps/${app}.png" \
        --plugin qt
}

# Makes the AppImage of an AppDir: ${BUILD_DIR}/<app>-<version>-x86_64.AppImage.
#   $1  the application
#   $2  its own AppRun, in place of linuxdeploy's
create_appimage() {
    local app="$1" apprun="$2"
    local appdir="${BUILD_DIR}/AppDir_${app}"
    local version
    version="$(get_version)"
    rm -f "${appdir}/AppRun"
    cp "${apprun}" "${appdir}/AppRun"
    chmod +x "${appdir}/AppRun"

    echo "=== Creating the AppImage"
    cd "${BUILD_DIR}"
    rm -f ./*.AppImage
    VERSION="${version}" linuxdeploy-x86_64.AppImage --appdir "${appdir}" --output appimage > /dev/null
    # linuxdeploy names it after the desktop file's Name: renamed after the program.
    local made
    made="$(ls -t ./*.AppImage | head -1)"
    mv "${made}" "${app}-${version}-x86_64.AppImage"
    echo "=== ${BUILD_DIR}/${app}-${version}-x86_64.AppImage"
}
