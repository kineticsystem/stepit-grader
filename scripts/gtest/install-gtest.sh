#!/usr/bin/env bash
# GoogleTest: the tests. Static, without GoogleMock, which they do not use.

install-gtest() {
    local name=gtest version=1.15.2
    installed $name && return

    echo "$name $version"
    local source
    source="$(fetch "https://github.com/google/googletest/releases/download/v$version/googletest-$version.tar.gz" \
        7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926 "googletest-$version.tar.gz")"
    cmake_install "$source" $name -DBUILD_GMOCK=OFF -DINSTALL_GTEST=ON -Dgtest_force_shared_crt=ON
}

install-gtest
