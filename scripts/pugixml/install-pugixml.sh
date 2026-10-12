#!/usr/bin/env bash
# pugixml: the XML of the sidecars. Static.

install-pugixml() {
    local name=pugixml version=1.15
    installed $name && return

    echo "$name $version"
    local source
    source="$(fetch "https://github.com/zeux/pugixml/releases/download/v$version/pugixml-$version.tar.gz" \
        655ade57fa703fb421c2eb9a0113b5064bddb145d415dd1f88c79353d90d511a "pugixml-$version.tar.gz")"
    cmake_install "$source" $name -DPUGIXML_BUILD_TESTS=OFF
}

install-pugixml
