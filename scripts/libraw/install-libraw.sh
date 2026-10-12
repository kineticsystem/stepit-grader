#!/usr/bin/env bash
# LibRaw: the decoding and demosaicing of camera raw files and DNGs, in
# raw-converter. Shared, as its LGPL licence asks of a program that links it;
# found with pkg-config, as libraw_r. Needs Little CMS.

install-libraw() {
    local name=libraw version=0.21.4
    add_path PKG_CONFIG_PATH "$DEPS_DIR/$name/lib/pkgconfig"
    installed $name && return

    echo "$name $version"
    local source
    source="$(fetch "https://www.libraw.org/data/LibRaw-$version.tar.gz" \
        6be43f19397e43214ff56aab056bf3ff4925ca14012ce5a1538a172406a09e63 "LibRaw-$version.tar.gz")"
    # No OpenMP: Apple's clang has none, and the editor runs its own threads.
    configure_install "$source" $name --disable-examples --disable-openmp --enable-lcms
}

install-libraw
