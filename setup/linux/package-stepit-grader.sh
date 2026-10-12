#!/usr/bin/env bash
# Makes StepIt Grader's AppImage: build/linux-package/stepit-grader-<version>-x86_64.AppImage,
# with the editor and the command line. Run it from anywhere.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

build_project
setup_linuxdeploy
deploy_appdir stepit-grader "stepit-grader-cli"
create_appimage stepit-grader "${SCRIPT_DIR}/AppRun-stepit-grader.sh"
