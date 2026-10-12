#!/usr/bin/env bash
# linuxdeploy and its Qt plugin: the tools that make the Linux AppImage, see
# setup/linux. Linux only; their latest builds, which they publish as
# "continuous".

install-linuxdeploy() {
    local name=linuxdeploy
    [[ "$(os_type)" == "linux" ]] || return 0
    installed $name && return

    echo "$name"
    local base=https://github.com/linuxdeploy tool
    mkdir -p "$DEPS_DIR/$name.part"
    for tool in linuxdeploy linuxdeploy-plugin-qt; do
        curl -fsSL --retry 3 -o "$DEPS_DIR/$name.part/$tool-x86_64.AppImage" \
            "$base/$tool/releases/download/continuous/$tool-x86_64.AppImage"
        chmod +x "$DEPS_DIR/$name.part/$tool-x86_64.AppImage"
    done
    # Whole, or not at all: a download cut short is not taken for installed.
    mv "$DEPS_DIR/$name.part" "$DEPS_DIR/$name"
}

install-linuxdeploy
