// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "imageio/ImageReader.h"

#include <algorithm>
#include <cctype>

namespace imageio {

std::string extensionOf(const std::filesystem::path &path)
{
    std::string ext = path.extension().string();
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(0, 1);
    }
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext;
}

bool ImageReader::canRead(const std::filesystem::path &path) const
{
    return std::ranges::contains(extensions(), extensionOf(path));
}

} // namespace imageio
