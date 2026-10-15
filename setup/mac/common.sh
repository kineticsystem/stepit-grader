#!/usr/bin/env bash
# What the macOS packaging scripts share. Sourced, not run.
#
# Needs Qt6_DIR, e.g. ~/Qt/6.11.0/macos/lib/cmake/Qt6, and the libraries of
# install-deps.sh in deps/, built for both architectures. The bundle is
# universal, for Apple Silicon and Intel, and signed ad hoc only.
#
#   build_project_once
#   find_qmake
#   create_dmg_for_app <app> <BundleName> <bundle id> <icon.icns> [other programs]

error() { echo "ERROR: $*" >&2; }
warning() { echo "WARNING: $*" >&2; }

COMMON_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PROJECT_ROOT="$(cd "${COMMON_SCRIPT_DIR}/../.." && pwd)"
# A build of its own, apart from the development build.
export BUILD_DIR="${PROJECT_ROOT}/build/macos-package"
# The oldest macOS it runs on, CMAKE_OSX_DEPLOYMENT_TARGET of CMakeLists.txt,
# which the build and the libraries of deps/ use too.
export MACOS_DEPLOYMENT_TARGET="$(sed -n 's/^ *set(CMAKE_OSX_DEPLOYMENT_TARGET "\([0-9.]*\)".*/\1/p' \
    "${PROJECT_ROOT}/CMakeLists.txt")"
# Apple Silicon and Intel, in one program.
export MACOS_ARCHITECTURES="arm64;x86_64"

# The version of the project, from its CMakeLists.txt, as for the AppImage.
get_version() {
    sed -n 's/^ *VERSION \([0-9.]*\).*/\1/p' "${PROJECT_ROOT}/CMakeLists.txt" | head -1
}

# Builds the project once: remove ${BUILD_DIR}/.build_complete to build it again.
# The tests are built too, to be shipped in the bundle and run there, with its
# libraries.
build_project_once() {
    if [[ -f "${BUILD_DIR}/.build_complete" && -d "${BUILD_DIR}/bin/stepit-grader.app" &&
        -f "${BUILD_DIR}/bin/stepit-grader-tests" ]]; then
        echo "Project already built. To build it again: rm ${BUILD_DIR}/.build_complete"
        return 0
    fi
    if [[ -z "${Qt6_DIR:-}" ]]; then
        error "Qt6_DIR is not set, e.g. export Qt6_DIR=~/Qt/6.11.0/macos/lib/cmake/Qt6"
        exit 1
    fi
    echo "=== Building into ${BUILD_DIR}"
    cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES="${MACOS_ARCHITECTURES}" \
        -DSTEPIT_GRADER_BUILD_TESTS=ON
    cmake --build "${BUILD_DIR}" --parallel
    touch "${BUILD_DIR}/.build_complete"
}

# qmake, which tells where Qt is, and macdeployqt, from Qt6_DIR.
find_qmake() {
    QT_BASE_DIR="${Qt6_DIR%%/lib/cmake*}"
    if [[ ! -x "${QT_BASE_DIR}/bin/qmake" ]]; then
        error "Cannot find qmake at ${QT_BASE_DIR}/bin/qmake: check Qt6_DIR"
        exit 1
    fi
    export QT_BASE_DIR
    export QMAKE="${QT_BASE_DIR}/bin/qmake"
    export MACDEPLOYQT="${QT_BASE_DIR}/bin/macdeployqt"
}

# Copies the libraries of deps/ that the bundled ones need and macdeployqt
# missed: it follows the programs' own libraries, not those of their libraries.
copy_missing_dependencies() {
    local frameworks_dir="$1"
    local copied=1
    echo "Checking for missing indirect dependencies..."
    while [[ "$copied" -gt 0 ]]; do
        copied=0
        for dylib in "${frameworks_dir}"/*.dylib; do
            [[ -f "$dylib" ]] || continue
            while IFS= read -r dep; do
                [[ -z "$dep" || -f "${frameworks_dir}/${dep}" ]] && continue
                local found
                found=$(find "${PROJECT_ROOT}/deps" -maxdepth 3 -path "*/lib/${dep}" \
                    \( -type f -o -type l \) 2>/dev/null | head -1) || true
                if [[ -n "$found" ]]; then
                    echo "  Copying missing dependency: ${dep}"
                    cp -L "$found" "${frameworks_dir}/"
                    chmod 755 "${frameworks_dir}/${dep}"
                    copied=$((copied + 1))
                fi
            done < <(otool -L "$dylib" 2>/dev/null | grep -E '@(rpath|loader_path)/' |
                awk '{print $1}' | sed -e 's|@rpath/||' -e 's|@loader_path/||')
        done
    done
}

# Makes <BundleName>.app and <BundleName>-<version>.dmg in ${BUILD_DIR}.
#   $1  the program, e.g. stepit-grader
#   $2  the bundle's name, e.g. StepItGrader
#   $3  the bundle's identifier
#   $4  the icon, in src/editor-app/resources
#   $5  other programs of ${BUILD_DIR}/bin to put beside it, space separated
create_dmg_for_app() {
    local app_name="$1" bundle_name="$2" bundle_id="$3" icon_file="$4" others="${5:-}"
    local version
    version="$(get_version)"
    local app_bundle="${bundle_name}.app"
    local dmg_name="${bundle_name}-${version}-macos.dmg"

    echo ""
    echo "=== Creating DMG for: ${app_name}"
    cd "${BUILD_DIR}"
    rm -rf "${app_bundle}" "${dmg_name}"
    mkdir -p "${app_bundle}/Contents/MacOS" "${app_bundle}/Contents/Frameworks" \
        "${app_bundle}/Contents/Resources" "${app_bundle}/Contents/PlugIns"

    echo "Copying the programs..."
    cp "${BUILD_DIR}/bin/${app_name}.app/Contents/MacOS/${app_name}" "${app_bundle}/Contents/MacOS/"
    chmod 755 "${app_bundle}/Contents/MacOS/${app_name}"
    local other executables=()
    for other in ${others}; do
        cp "${BUILD_DIR}/bin/${other}" "${app_bundle}/Contents/MacOS/"
        chmod 755 "${app_bundle}/Contents/MacOS/${other}"
        executables+=("-executable=${app_bundle}/Contents/MacOS/${other}")
    done

    cp "${PROJECT_ROOT}/src/editor-app/resources/${icon_file}" "${app_bundle}/Contents/Resources/"

    echo "Creating Info.plist..."
    cat > "${app_bundle}/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>NSPrincipalClass</key>
	<string>NSApplication</string>

	<key>CFBundleShortVersionString</key>
	<string>${version}</string>

	<key>CFBundleVersion</key>
	<string>${version}</string>

	<key>NSHumanReadableCopyright</key>
	<string>Copyright © 2026 Giovanni Remigi. MIT License.</string>

	<key>CFBundleIconFile</key>
	<string>${icon_file}</string>

	<key>CFBundleName</key>
	<string>StepIt Grader</string>

	<key>CFBundleDisplayName</key>
	<string>StepIt Grader</string>

	<key>CFBundlePackageType</key>
	<string>APPL</string>

	<key>CFBundleSignature</key>
	<string>????</string>

	<key>CFBundleExecutable</key>
	<string>${app_name}</string>

	<key>CFBundleIdentifier</key>
	<string>${bundle_id}</string>

	<key>LSMinimumSystemVersion</key>
	<string>${MACOS_DEPLOYMENT_TARGET}</string>

	<key>NSHighResolutionCapable</key>
	<true/>
</dict>
</plist>
EOF

    echo "Creating qt.conf..."
    cat > "${app_bundle}/Contents/Resources/qt.conf" <<EOF
[Paths]
Plugins = PlugIns
EOF

    echo "Running macdeployqt..."
    if [[ ! -x "$MACDEPLOYQT" ]]; then
        error "macdeployqt not found at $MACDEPLOYQT"
        exit 1
    fi
    # The libraries of deps/, for macdeployqt to find.
    local libpaths=() lib
    for lib in "${PROJECT_ROOT}"/deps/*/lib; do
        libpaths+=("-libpath=${lib}")
    done
    "$MACDEPLOYQT" "${app_bundle}" -verbose=1 "${executables[@]}" "${libpaths[@]}"

    # macdeployqt skips the SVG image format plugin, which depends on QtSvg.
    local svg_plugin="${QT_BASE_DIR}/plugins/imageformats/libqsvg.dylib"
    local imageformats_dir="${app_bundle}/Contents/PlugIns/imageformats"
    if [[ -f "$svg_plugin" && ! -f "${imageformats_dir}/libqsvg.dylib" ]]; then
        echo "Copying the SVG image format plugin..."
        mkdir -p "${imageformats_dir}"
        cp "$svg_plugin" "${imageformats_dir}/"
        chmod 755 "${imageformats_dir}/libqsvg.dylib"
    fi

    # macdeployqt deploys only the Cocoa platform; the tests run offscreen.
    local offscreen_plugin="${QT_BASE_DIR}/plugins/platforms/libqoffscreen.dylib"
    local platforms_dir="${app_bundle}/Contents/PlugIns/platforms"
    if [[ " ${others} " == *" stepit-grader-tests "* && -f "$offscreen_plugin" &&
        ! -f "${platforms_dir}/libqoffscreen.dylib" ]]; then
        echo "Copying the offscreen platform plugin, for the tests..."
        mkdir -p "${platforms_dir}"
        cp "$offscreen_plugin" "${platforms_dir}/"
        chmod 755 "${platforms_dir}/libqoffscreen.dylib"
    fi

    # Our libraries, not any of the same name that macdeployqt took from Qt.
    local frameworks_dir="${app_bundle}/Contents/Frameworks"
    echo "Ensuring the project's libraries are used..."
    for src_lib in "${PROJECT_ROOT}"/deps/*/lib/*.dylib; do
        [[ -e "$src_lib" ]] || continue
        local lib_name
        lib_name=$(basename "$src_lib")
        if [[ -f "${frameworks_dir}/${lib_name}" ]]; then
            echo "  Replacing ${lib_name} with the project's"
            cp -L -f "$src_lib" "${frameworks_dir}/${lib_name}"
            chmod 755 "${frameworks_dir}/${lib_name}"
        fi
    done

    # The libraries' paths on the build machine become @rpath.
    echo "Fixing library install names..."
    for dylib in "${frameworks_dir}"/*.dylib; do
        [[ -f "$dylib" ]] || continue
        local lib_name
        lib_name=$(basename "$dylib")
        install_name_tool -id "@rpath/${lib_name}" "$dylib" 2>/dev/null || true
        otool -L "$dylib" 2>/dev/null | awk 'NR>1 {print $1}' | while read -r dep; do
            case "$dep" in
                /Users/* | /opt/* | /usr/local/*)
                    install_name_tool -change "$dep" "@rpath/$(basename "$dep")" "$dylib" \
                        2>/dev/null || true
                    ;;
            esac
        done
    done

    # The build machine's run paths do not exist where the bundle goes.
    echo "Cleaning up RPATH entries..."
    local program
    for program in "${app_bundle}"/Contents/MacOS/*; do
        while IFS= read -r rpath; do
            if [[ "$rpath" != @* ]]; then
                echo "  Removing RPATH from $(basename "$program"): $rpath"
                install_name_tool -delete_rpath "$rpath" "$program" 2>/dev/null || true
            fi
        done < <(otool -l "$program" | grep -A2 LC_RPATH | grep "path " | awk '{print $2}')
    done

    copy_missing_dependencies "${frameworks_dir}"

    # Signed ad hoc, as macOS requires after macdeployqt changed the files: the
    # first opening asks the user to allow it. Libraries and plugins first.
    echo "Signing app bundle..."
    local sign_errors=0 file
    while IFS= read -r -d '' file; do
        codesign --force --sign - "$file" 2>/dev/null || {
            warning "Failed to sign $file"
            sign_errors=$((sign_errors + 1))
        }
    done < <(find "${app_bundle}/Contents/Frameworks" "${app_bundle}/Contents/PlugIns" -type f \
        \( -name "*.dylib" -o -perm +111 \) -print0 2>/dev/null)
    while IFS= read -r -d '' file; do
        codesign --force --sign - "$file" 2>/dev/null || {
            warning "Failed to sign $file"
            sign_errors=$((sign_errors + 1))
        }
    done < <(find "${app_bundle}/Contents/Frameworks" -type d -name "*.framework" -print0 2>/dev/null)
    codesign --force --deep --sign - "${app_bundle}"
    echo "App bundle signed, with ${sign_errors} warnings"

    # Both architectures, or the bundle is not what it claims.
    lipo -info "${app_bundle}/Contents/MacOS/${app_name}"

    echo "Creating DMG..."
    hdiutil create -volname "StepIt Grader" -srcfolder "${app_bundle}" -ov -format UDZO "${dmg_name}"
    echo "=== ${BUILD_DIR}/${dmg_name}"
}
