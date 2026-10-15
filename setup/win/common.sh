#!/usr/bin/env bash
# What the Windows packaging scripts share, in an MSYS2 MINGW64 shell. Sourced,
# not run.
#
# Needs MSYS2's Qt 6 with its tools (windeployqt6), the libraries of
# install-deps.sh in deps/, and, for the installer, Inno Setup's ISCC.exe on
# the PATH.

# Get the directory where this script is located
COMMON_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Calculate project root (two levels up from setup/win/)
export PROJECT_ROOT="$( cd "${COMMON_SCRIPT_DIR}/../.." && pwd )"

# Ensure MSYS2 MinGW64 tools are in PATH
# There may be multiple mingw64 installations (e.g., /mingw64 mount and /c/msys64/mingw64).
# Prefer the one that has the required Qt6 tools (windeployqt6).
if [ -f "/c/msys64/mingw64/bin/windeployqt6.exe" ]; then
    # Use the full installation with Qt6 tools
    export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH"
elif [ -n "${MINGW_PREFIX:-}" ] && [ -f "${MINGW_PREFIX}/bin/windeployqt6.exe" ]; then
    export PATH="${MINGW_PREFIX}/bin:/usr/bin:$PATH"
elif [ -f "/mingw64/bin/windeployqt6.exe" ]; then
    export PATH="/mingw64/bin:/usr/bin:$PATH"
else
    echo "ERROR: Cannot find windeployqt6. Install Qt6 in MSYS2: pacman -S mingw-w64-x86_64-qt6-tools"
    exit 1
fi

# Set temp directories to avoid permission issues with Windows temp
export TMPDIR="/tmp"
export TEMP="/tmp"
export TMP="/tmp"

# Set build directory in a subfolder to avoid conflicts with debug builds
export BUILD_DIR="${PROJECT_ROOT}/build/win-package"

# ##############################################################################
# DLL Dependency Scanner
# ##############################################################################
#
# Discovers DLL dependencies needed for a Windows application by recursively
# scanning the executable and its dependencies using ldd. Similar to the macOS
# approach with otool -L, this iteratively finds transitive dependencies.
#
# Usage: scan_required_dlls APP_NAME [BUILD_DIR]
#   APP_NAME  - Name of the executable without .exe extension
#   BUILD_DIR - Build directory containing bin/{APP_NAME}.exe (default: $BUILD_DIR)
#
# Sets: REQUIRED_DLLS array with all non-Qt DLLs needed
#
scan_required_dlls() {
    local app_name="$1"
    local build_dir="${2:-$BUILD_DIR}"

    echo "Scanning DLL dependencies for ${app_name}..."

    # Create temp directory
    local scan_temp=$(mktemp -d)
    trap 'rm -rf "$scan_temp"' RETURN

    # 1. Get Qt DLLs (to exclude them - windeployqt handles these)
    echo "  Step 1: Identifying Qt DLLs..."
    local deploy="$scan_temp/deploy"
    mkdir -p "$deploy"
    cp "${build_dir}/bin/${app_name}.exe" "$deploy/"

    pushd "$deploy" >/dev/null
    windeployqt6 --release --no-translations "${app_name}.exe" >/dev/null 2>&1 || \
        windeployqt --release --no-translations "${app_name}.exe" >/dev/null 2>&1
    popd >/dev/null

    # Build Qt DLL list (lowercase for case-insensitive comparison)
    # We'll use this to identify Qt DLLs, but we still need to scan them for non-Qt dependencies
    declare -A qt_dlls
    declare -a qt_dll_paths  # Full paths to Qt DLLs for scanning
    while IFS= read -r dll_path; do
        local dll_name=$(basename "$dll_path")
        qt_dlls["$(echo "$dll_name" | tr '[:upper:]' '[:lower:]')"]=1
        qt_dll_paths+=("$dll_path")
    done < <(find "$deploy" -name "*.dll" 2>/dev/null)
    echo "    Qt: ${#qt_dlls[@]} DLLs identified"

    # 2. Build lookup table of available lib DLLs
    echo "  Step 2: Indexing deps/ directories..."
    declare -A lib_dll_paths  # dll_name -> full_path
    for lib_dir in "${PROJECT_ROOT}"/deps/*/bin "${PROJECT_ROOT}"/deps/*/lib; do
        if [ -d "$lib_dir" ]; then
            for dll in "$lib_dir"/*.dll; do
                if [ -f "$dll" ]; then
                    local dll_name=$(basename "$dll")
                    lib_dll_paths["$dll_name"]="$dll"
                fi
            done
        fi
    done
    echo "    Indexed: ${#lib_dll_paths[@]} DLLs from deps/"

    # 3. Recursive dependency scanning (similar to macOS copy_missing_dependencies)
    echo "  Step 3: Scanning transitive dependencies recursively..."

    declare -A needed_dlls      # DLLs we need to include
    declare -A scanned_files    # Files we've already scanned
    local iteration=0
    local new_deps=1

    # Start with the executable AND the Qt DLLs (to find their non-Qt dependencies)
    local files_to_scan=("${build_dir}/bin/${app_name}.exe")
    for qt_path in "${qt_dll_paths[@]}"; do
        files_to_scan+=("$qt_path")
    done

    while [ "$new_deps" -gt 0 ]; do
        new_deps=0
        iteration=$((iteration + 1))
        echo "    Iteration $iteration: scanning ${#files_to_scan[@]} file(s)..."

        local next_files=()

        for file in "${files_to_scan[@]}"; do
            [ -f "$file" ] || continue

            local file_key=$(basename "$file")
            [ -n "${scanned_files[$file_key]:-}" ] && continue
            scanned_files["$file_key"]=1

            # Get dependencies using ldd
            while IFS= read -r line; do
                # Parse ldd output - handle multiple formats:
                # Format 1: "libfoo.dll => /mingw64/bin/libfoo.dll (0x...)"
                # Format 2: "libfoo.dll => not found"
                local dll_name=""
                local dll_path=""
                local not_found=false

                # Extract DLL name (first field)
                dll_name=$(echo "$line" | awk '{print $1}')

                # Skip if not a DLL
                [ -z "$dll_name" ] || [[ "$dll_name" != *.dll ]] && continue

                # Check if it's "not found" or has a path
                if echo "$line" | grep -q "=> not found" ; then
                    not_found=true
                elif echo "$line" | grep -qE "=> /" ; then
                    dll_path=$(echo "$line" | awk '{print $3}')
                else
                    # Skip lines that don't match expected patterns
                    continue
                fi

                # Skip Qt DLLs (case-insensitive)
                local dll_lower=$(echo "$dll_name" | tr '[:upper:]' '[:lower:]')
                [ -n "${qt_dlls[$dll_lower]:-}" ] && continue

                # Skip system DLLs (Windows system, msvcrt, etc.)
                if [ -n "$dll_path" ] && echo "$dll_path" | grep -qiE "^/(c/)?windows/" ; then
                    continue
                fi

                # Skip if already in needed list
                [ -n "${needed_dlls[$dll_name]:-}" ] && continue

                # Check if this DLL comes from our lib directories (priority)
                if [ -n "${lib_dll_paths[$dll_name]:-}" ]; then
                    needed_dlls["$dll_name"]="${lib_dll_paths[$dll_name]}"
                    next_files+=("${lib_dll_paths[$dll_name]}")
                    new_deps=$((new_deps + 1))
                # Or from mingw64/bin (system runtime dependencies)
                elif [ -f "/mingw64/bin/$dll_name" ]; then
                    needed_dlls["$dll_name"]="/mingw64/bin/$dll_name"
                    next_files+=("/mingw64/bin/$dll_name")
                    new_deps=$((new_deps + 1))
                elif [ -f "/c/msys64/mingw64/bin/$dll_name" ]; then
                    needed_dlls["$dll_name"]="/c/msys64/mingw64/bin/$dll_name"
                    next_files+=("/c/msys64/mingw64/bin/$dll_name")
                    new_deps=$((new_deps + 1))
                elif [ "$not_found" = true ]; then
                    # Skip Windows API sets (api-ms-win-*) - these are provided by Windows
                    if [[ "$dll_name" == api-ms-win-* ]]; then
                        continue
                    fi
                    # DLL not found anywhere - will be reported as missing later
                    echo "      Warning: $dll_name not found in deps/ or mingw64"
                fi
            done < <(ldd "$file" 2>/dev/null || true)
        done

        files_to_scan=("${next_files[@]}")
        echo "      Found $new_deps new dependencies"
    done

    echo "    Total iterations: $iteration"

    # 4. Add runtime plugins (DLLs loaded dynamically, not detected by ldd)
    echo "  Step 4: Adding runtime plugins..."
    local runtime_plugins_added=0

    echo "    Runtime plugins: $runtime_plugins_added"
    echo "    Required: ${#needed_dlls[@]} DLLs (excluding Qt)"

    # 5. Populate REQUIRED_DLLS array (just the names, paths stored separately)
    REQUIRED_DLLS=()
    for dll_name in "${!needed_dlls[@]}"; do
        REQUIRED_DLLS+=("$dll_name")
    done

    # Store paths for use by collect_dependencies
    declare -gA REQUIRED_DLL_PATHS
    for dll_name in "${!needed_dlls[@]}"; do
        REQUIRED_DLL_PATHS["$dll_name"]="${needed_dlls[$dll_name]}"
    done

    echo "  Scan complete."
    echo ""
}

# ##############################################################################
# Common Functions
# ##############################################################################

build_project_once() {
    # Check if already built
    if [ -f "${BUILD_DIR}/.build_complete" ] && [ -f "${BUILD_DIR}/bin/stepit-grader-tests.exe" ]; then
        echo "Project already built. Skipping build step."
        echo "To force rebuild, run: rm ${BUILD_DIR}/.build_complete"
        return 0
    fi
    
    echo "=== Building Project ==="
    echo "Project root: ${PROJECT_ROOT}"
    echo "Build directory: ${BUILD_DIR}"
    echo ""

    # Create and enter build directory
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    # Check if we need to clean due to generator mismatch
    if [ -f "CMakeCache.txt" ]; then
        CACHED_GENERATOR=$(grep "CMAKE_GENERATOR:INTERNAL=" CMakeCache.txt | cut -d'=' -f2)
        
        # On Windows/MSYS2, prefer Ninja if available, otherwise MinGW Makefiles
        if command -v ninja &> /dev/null; then
            DESIRED_GENERATOR="Ninja"
            BUILD_COMMAND="ninja"
        else
            DESIRED_GENERATOR="MinGW Makefiles"
            BUILD_COMMAND="mingw32-make"
        fi
        
        if [ "$CACHED_GENERATOR" != "$DESIRED_GENERATOR" ] && [ -n "$CACHED_GENERATOR" ]; then
            echo "Cleaning build directory due to generator mismatch (was: $CACHED_GENERATOR, want: $DESIRED_GENERATOR)..."
            rm -rf CMakeCache.txt CMakeFiles
        fi
    else
        # First time configuring
        if command -v ninja &> /dev/null; then
            DESIRED_GENERATOR="Ninja"
            BUILD_COMMAND="ninja"
        else
            DESIRED_GENERATOR="MinGW Makefiles"
            BUILD_COMMAND="mingw32-make"
        fi
    fi

    # Configure CMake
    echo "Configuring CMake with generator: ${DESIRED_GENERATOR}..."
    cmake "${PROJECT_ROOT}" \
        -G "${DESIRED_GENERATOR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSTEPIT_GRADER_BUILD_TESTS=ON

    # Build
    echo "Building with ${BUILD_COMMAND}..."
    ${BUILD_COMMAND} -j$(nproc)

    # Install into staging directory
    echo "Installing to staging directory..."
    rm -rf staging
    
    if [ "$BUILD_COMMAND" = "ninja" ]; then
        DESTDIR=staging ninja install
    else
        mingw32-make install DESTDIR=staging
    fi
    
    # Mark build as complete
    touch "${BUILD_DIR}/.build_complete"
}

find_dll() {
    local dll_name="$1"

    # Search in project lib directories first
    for lib_dir in "${PROJECT_ROOT}"/deps/*/bin "${PROJECT_ROOT}"/deps/*/lib; do
        if [ -f "${lib_dir}/${dll_name}" ]; then
            echo "${lib_dir}/${dll_name}"
            return 0
        fi
    done

    # Search in MSYS2 MinGW64 bin directory
    if [ -f "/c/msys64/mingw64/bin/${dll_name}" ]; then
        echo "/c/msys64/mingw64/bin/${dll_name}"
        return 0
    fi

    # Not found
    return 1
}

collect_dependencies() {
    local app_name="$1"
    local deploy_dir="$2"
    local extra_files="${3:-}"  # Optional: space-separated list of extra files (fonts, resources) to include

    # Disable exit-on-error for this function
    set +e

    echo ""
    echo "=== Collecting Dependencies for: ${app_name} ==="

    cd "${BUILD_DIR}"

    # Create deployment directory
    mkdir -p "${deploy_dir}"

    # Copy the executable
    echo "Copying executable..."
    cp -v "bin/${app_name}.exe" "${deploy_dir}/"

    # Copy extra files if specified
    if [ -n "${extra_files}" ]; then
        echo ""
        echo "Copying extra files..."
        for extra in ${extra_files}; do
            if [ -f "bin/${extra}" ]; then
                cp -v "bin/${extra}" "${deploy_dir}/"
                echo "  Including extra file: ${extra}"
            else
                echo "  Warning: Extra file not found: bin/${extra}"
            fi
        done
    fi

    # Run windeployqt to collect Qt dependencies
    echo ""
    echo "Running windeployqt to collect Qt dependencies..."
    cd "${deploy_dir}"
    windeployqt6 --release --no-translations "${app_name}.exe" 2>/dev/null || windeployqt --release --no-translations "${app_name}.exe"
    # The Qt modules of the extra programs too, e.g. Qt Test for the tests.
    for extra in ${extra_files}; do
        if [[ "${extra}" == *.exe && -f "${extra}" ]]; then
            windeployqt6 --release --no-translations "${extra}" >/dev/null 2>&1 || true
        fi
    done
    cd "${BUILD_DIR}"

    # Copy required DLLs (REQUIRED_DLLS and REQUIRED_DLL_PATHS must be defined by scan_required_dlls)
    if [ ${#REQUIRED_DLLS[@]} -gt 0 ]; then
        echo ""
        echo "Copying required DLLs (${#REQUIRED_DLLS[@]} total)..."
        local copied_count=0
        local already_present=0
        local missing_count=0
        declare -a missing_list

        for dll in "${REQUIRED_DLLS[@]}"; do
            # Skip empty entries
            [ -z "$dll" ] && continue

            # Skip if already present (windeployqt may have copied some)
            if [ -f "${deploy_dir}/${dll}" ]; then
                ((already_present++))
                continue
            fi

            # Get the path from our scan results
            local dll_path="${REQUIRED_DLL_PATHS[$dll]:-}"

            if [ -n "$dll_path" ] && [ -f "$dll_path" ]; then
                if cp "$dll_path" "${deploy_dir}/" 2>/dev/null; then
                    ((copied_count++))
                else
                    echo "    ERROR: Failed to copy $dll"
                    missing_list+=("$dll")
                    ((missing_count++))
                fi
            else
                # Fallback to find_dll if path wasn't stored
                dll_path=$(find_dll "$dll")
                if [ $? -eq 0 ] && [ -n "$dll_path" ] && [ -f "$dll_path" ]; then
                    if cp "$dll_path" "${deploy_dir}/" 2>/dev/null; then
                        ((copied_count++))
                    else
                        echo "    ERROR: Failed to copy $dll"
                        missing_list+=("$dll")
                        ((missing_count++))
                    fi
                else
                    missing_list+=("$dll")
                    ((missing_count++))
                fi
            fi
        done

        echo ""
        echo "DLL Copy Summary:"
        echo "  - Already present: $already_present"
        echo "  - Newly copied: $copied_count"
        echo "  - Not found: $missing_count"

        if [ $missing_count -gt 0 ]; then
            echo ""
            echo "WARNING: Missing DLLs:"
            for missing_dll in "${missing_list[@]}"; do
                echo "    - $missing_dll"
            done
            echo ""
            echo "To fix: Install missing packages with: pacman -S mingw-w64-x86_64-<package>"
        fi
    else
        echo ""
        echo "WARNING: No REQUIRED_DLLS array defined - run scan_required_dlls first."
    fi

    echo ""
    echo "=== Dependencies collected: ${deploy_dir} ==="
    cd "${deploy_dir}"
    echo "Directory size: $(du -sh . 2>/dev/null | cut -f1)"
    echo "Total files: $(find . -type f 2>/dev/null | wc -l)"
    echo "DLL count: $(find . -name "*.dll" 2>/dev/null | wc -l)"

    # Final verification - check all DLLs in the deployment directory
    echo ""
    echo "=== Verifying Dependencies ==="

    local all_satisfied=true
    local missing_deps=""

    for file in "${deploy_dir}"/*.exe "${deploy_dir}"/*.dll; do
        [ -f "$file" ] || continue
        local file_missing=$(ldd "$file" 2>/dev/null | grep "not found" || true)
        if [ -n "$file_missing" ]; then
            all_satisfied=false
            missing_deps="${missing_deps}$(basename "$file"):\n${file_missing}\n"
        fi
    done

    if [ "$all_satisfied" = true ]; then
        echo "All dependencies satisfied"
    else
        echo "WARNING: Missing dependencies found:"
        echo -e "$missing_deps"
    fi

    cd "${BUILD_DIR}"

    # Re-enable exit-on-error
    set -e
}

# The version of the project, from its CMakeLists.txt, as for the AppImage.
get_version() {
    sed -n 's/^ *VERSION \([0-9.]*\).*/\1/p' "${PROJECT_ROOT}/CMakeLists.txt" | head -1
}

create_portable_package() {
    local app_name="$1"
    local deploy_dir="$2"
    local zip_name="${3:-${app_name}-portable-win64.zip}"

    echo ""
    echo "=== Creating Portable Package for: ${app_name} ==="

    cd "${BUILD_DIR}"

    if command -v 7z &> /dev/null; then
        echo "Creating archive with 7-Zip..."
        rm -f "${zip_name}"
        7z a -tzip "${zip_name}" "./${deploy_dir}/*" >/dev/null 2>&1
    elif command -v zip &> /dev/null; then
        echo "Creating archive with zip..."
        rm -f "${zip_name}"
        cd "${deploy_dir}"
        zip -r -q "../${zip_name}" ./*
        cd ..
    else
        echo "WARNING: Neither 7z nor zip found. Skipping archive creation."
        return 1
    fi

    if [ -f "${zip_name}" ]; then
        echo "✓ Portable package created: ${zip_name}"
        ls -lh "${zip_name}"
    fi
}
