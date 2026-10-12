#!/usr/bin/env bash
# StepIt Grader's AppImage: the editor, or, with --cli first, the command line.
#
#   StepIt_Grader.AppImage [picture]
#   StepIt_Grader.AppImage --cli --settings look.xmp --output graded/ pictures...

HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/usr/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

if [[ "${1:-}" == "--cli" ]]; then
    shift
    exec "${HERE}/usr/bin/stepit-grader-cli" "$@"
fi
exec "${HERE}/usr/bin/stepit-grader" "$@"
