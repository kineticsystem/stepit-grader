#!/usr/bin/env bash
# Little CMS: the colour conversions of raw-converter and of the TIFF reader
# and writer, from a file's ICC profile. Shared; found with pkg-config.

install-lcms2() {
    local name=lcms2 version=2.17
    add_path PKG_CONFIG_PATH "$DEPS_DIR/$name/lib/pkgconfig"
    installed $name && return

    echo "$name $version"
    local source
    source="$(fetch "https://github.com/mm2/Little-CMS/releases/download/lcms$version/lcms2-$version.tar.gz" \
        d11af569e42a1baa1650d20ad61d12e41af4fead4aa7964a01f93b08b53ab074 "lcms2-$version.tar.gz")"
    configure_install "$source" $name
}

install-lcms2
