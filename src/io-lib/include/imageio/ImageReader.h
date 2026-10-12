// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

#include <filesystem>
#include <string>
#include <vector>

namespace imageio {

/**
 * Reads an image file into the working colour space, linear Rec. 2020, turned upright. One per
 * kind of file; the editor sees only this interface.
 */
class ImageReader
{
public:
    virtual ~ImageReader() = default;

    /// The extensions it reads, lower case, without the dot, e.g. "tif".
    virtual std::vector<std::string> extensions() const = 0;

    /// Whether it reads the file, from its name.
    virtual bool canRead(const std::filesystem::path &path) const;

    /// Throws imageio::Error when the file cannot be read. Safe to call from any thread.
    virtual core::Image read(const std::filesystem::path &path) const = 0;
};

/// The extension of the path, lower case, without the dot.
std::string extensionOf(const std::filesystem::path &path);

} // namespace imageio
