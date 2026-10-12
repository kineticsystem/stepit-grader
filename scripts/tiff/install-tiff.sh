#!/usr/bin/env bash
# libtiff: the TIFF reader and writer. Static, with Deflate from the system's
# zlib: the codecs of other libraries, e.g. JPEG or WebP, are off, so none is
# found by chance on one system and not another.

install-tiff() {
    local name=tiff version=4.7.0
    installed $name && return

    echo "$name $version"
    local source
    source="$(fetch "https://download.osgeo.org/libtiff/tiff-$version.tar.gz" \
        67160e3457365ab96c5b3286a0903aa6e78bdc44c4bc737d2e486bcecb6ba976 "tiff-$version.tar.gz")"
    cmake_install "$source" $name \
        -Dzlib=ON -Dpixarlog=ON \
        -Djpeg=OFF -Dold-jpeg=OFF -Djbig=OFF -Dlerc=OFF -Dlzma=OFF -Dwebp=OFF -Dzstd=OFF \
        -Dlibdeflate=OFF -Dcxx=OFF \
        -Dtiff-tools=OFF -Dtiff-tests=OFF -Dtiff-contrib=OFF -Dtiff-docs=OFF
}

install-tiff
