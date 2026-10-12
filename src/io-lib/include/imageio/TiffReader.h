// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "imageio/ImageReader.h"

namespace imageio {

/**
 * Reads TIFF files: RGB or grey, 8 or 16 bits, or 32-bit float. The colours are converted from
 * the file's ICC profile; a file without one is taken for sRGB, or, in float, for linear
 * Rec. 2020. The extra channels, e.g. alpha, are ignored.
 */
class TiffReader : public ImageReader
{
public:
    std::vector<std::string> extensions() const override { return {"tif", "tiff"}; }
    core::Image read(const std::filesystem::path &path) const override;
};

} // namespace imageio
