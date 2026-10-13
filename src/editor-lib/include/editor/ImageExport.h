// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "core/Pipeline.h"
#include "core/Settings.h"
#include "imageio/ColourSpace.h"

#include <QString>

#include <expected>
#include <vector>

/// A kind of file the edited picture can be saved as.
struct ImageFormat
{
    /// What the save dialog lists, e.g. "TIFF, 16 bits, sRGB (*.tif *.tiff)".
    QString filter;
    /// The extension given to a name without one, e.g. "tif".
    QString suffix;
    bool jpeg = false;
    imageio::ColourSpace colourSpace = imageio::ColourSpace::Srgb;
};

/// The formats, the first the default: 16-bit TIFFs in sRGB, Adobe RGB and ProPhoto RGB, a JPEG.
const std::vector<ImageFormat> &imageFormats();

/**
 * Grades the picture at its full size with `pipeline`, and writes it to `path` as `format` says:
 * a 16-bit TIFF, compressed without loss, with its ICC profile; or an 8-bit sRGB JPEG of high
 * quality. Runs on any thread.
 */
std::expected<void, QString> saveImage(
    const core::Image &picture,
    const core::Settings &settings,
    const core::Pipeline &pipeline,
    const QString &path,
    const ImageFormat &format);
