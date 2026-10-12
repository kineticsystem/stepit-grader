// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "imageio/ColourSpace.h"

#include <filesystem>
#include <string>

namespace imageio {

/// How a TIFF is written.
struct TiffOptions
{
    ColourSpace colourSpace = ColourSpace::Srgb;
    /// Deflate, lossless: about half the size, read by every program that reads TIFFs.
    bool compress = false;
    /// The TIFF's Software tag.
    std::string software = "StepIt Grader";
};

/**
 * Writes an image of the working space as a 16-bit RGB TIFF, converted to a colour space with
 * Little CMS, its ICC profile embedded. What is outside the colour space, or above white, is
 * clipped. The file is written whole under a temporary name, then renamed: a failure never
 * leaves half a file, nor harms the file it would replace.
 */
class TiffWriter
{
public:
    explicit TiffWriter(TiffOptions options = {});

    /// Throws imageio::Error when the file cannot be written.
    void write(const core::Image &image, const std::filesystem::path &path) const;

    const TiffOptions &options() const { return m_options; }

private:
    TiffOptions m_options;
};

} // namespace imageio
