#!/usr/bin/env bash
# Makes StepIt Grader's Windows packages, in an MSYS2 MINGW64 shell: the
# installer, build/win-package/StepItGrader-<version>-setup.exe, and the
# portable ZIP, build/win-package/StepItGrader-<version>-win64.zip, each with
# the editor and the command line. Run it from anywhere.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

APP_NAME="stepit-grader"
DEPLOY_DIR="deploy-${APP_NAME}"
# The command line, beside the editor; it needs no DLL the editor does not.
EXTRA_FILES="stepit-grader-cli.exe"

echo "========================================"
echo "  Building Windows Package: ${APP_NAME}"
echo "========================================"
echo ""

build_project_once
scan_required_dlls "${APP_NAME}"
rm -rf "${BUILD_DIR}/${DEPLOY_DIR}"
collect_dependencies "${APP_NAME}" "${DEPLOY_DIR}" "${EXTRA_FILES}"

VERSION=$(get_version)
echo ""
echo "Version: ${VERSION}"

create_portable_package "${APP_NAME}" "${DEPLOY_DIR}" "StepItGrader-${VERSION}-win64.zip"

echo ""
echo "=== Creating installer ==="
MSYS2_ARG_CONV_EXCL="/D" ISCC.exe "/DMyAppVersion=${VERSION}" "${SCRIPT_DIR}/setup.iss"

echo ""
echo "Portable package: ${BUILD_DIR}/StepItGrader-${VERSION}-win64.zip"
echo "Installer:        ${BUILD_DIR}/StepItGrader-${VERSION}-setup.exe"
