// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "imageio/ImageReader.h"

#include <memory>
#include <vector>

namespace imageio {

/// Reads a file with the first of its readers that reads it.
class ImageLoader : public ImageReader
{
public:
    explicit ImageLoader(std::vector<std::shared_ptr<const ImageReader>> readers);

    std::vector<std::string> extensions() const override;
    bool canRead(const std::filesystem::path &path) const override;
    core::Image read(const std::filesystem::path &path) const override;

private:
    std::vector<std::shared_ptr<const ImageReader>> m_readers;
};

/// The TIFF and the raw readers.
std::shared_ptr<const ImageReader> makeStandardImageReader();

} // namespace imageio
