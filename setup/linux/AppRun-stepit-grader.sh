#!/usr/bin/env bash
# StepIt Grader's AppImage: the editor, or, with --cli first, the command line,
# or, with --run-tests first, the tests, with the AppImage's own libraries.
#
#   StepIt_Grader.AppImage [picture]
#   StepIt_Grader.AppImage --cli --settings look.xmp --output graded/ pictures...
#   StepIt_Grader.AppImage --run-tests [--gtest_filter='EffectsTest.*']

HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/usr/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

if [[ "${1:-}" == "--cli" ]]; then
    shift
    exec "${HERE}/usr/bin/stepit-grader-cli" "$@"
fi
if [[ "${1:-}" == "--run-tests" ]]; then
    shift
    exec "${HERE}/usr/bin/stepit-grader-tests" "$@"
fi
exec "${HERE}/usr/bin/stepit-grader" "$@"
