#!/usr/bin/env bash
# libtiff: the TIFF reader and writer. Shared, as Qt's TIFF plugin, which the
# Linux AppImage bundles, needs it too. Deflate comes from the system's zlib;
# the codecs of other libraries, e.g. JPEG or WebP, are off, so none is found
# by chance on one system and not another.

install-tiff() {
    local name=tiff version=4.7.0
    add_path PKG_CONFIG_PATH "$DEPS_DIR/$name/lib/pkgconfig"
    installed $name && return

    echo "$name $version"
    local source
    source="$(fetch "https://download.osgeo.org/libtiff/tiff-$version.tar.gz" \
        67160e3457365ab96c5b3286a0903aa6e78bdc44c4bc737d2e486bcecb6ba976 "tiff-$version.tar.gz")"
    cmake_install "$source" $name -DBUILD_SHARED_LIBS=ON \
        -Dzlib=ON -Dpixarlog=ON \
        -Djpeg=OFF -Dold-jpeg=OFF -Djbig=OFF -Dlerc=OFF -Dlzma=OFF -Dwebp=OFF -Dzstd=OFF \
        -Dlibdeflate=OFF -Dcxx=OFF \
        -Dtiff-tools=OFF -Dtiff-tests=OFF -Dtiff-contrib=OFF -Dtiff-docs=OFF

    # Qt's TIFF plugin is built against libtiff.so.5; libtiff 4.x is
    # libtiff.so.6. The link lets the plugin load ours.
    if [[ -f "$DEPS_DIR/$name/lib/libtiff.so.6" ]]; then
        ln -sf libtiff.so.6 "$DEPS_DIR/$name/lib/libtiff.so.5"
    fi
}

install-tiff
