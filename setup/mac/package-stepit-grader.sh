#!/usr/bin/env bash
# Makes StepIt Grader's DMG: build/macos-package/StepItGrader-<version>-macos.dmg,
# a universal bundle with the editor, the command line, Contents/MacOS/
# stepit-grader-cli, and the tests, Contents/MacOS/stepit-grader-tests, which
# run with the bundle's libraries. Run it from anywhere.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

build_project_once
find_qmake
create_dmg_for_app stepit-grader StepItGrader io.github.kineticsystem.StepItGrader \
    stepit-grader.icns "stepit-grader-cli stepit-grader-tests"
