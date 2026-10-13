// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "imageio/RawReader.h"
#include "imageio/Error.h"

#include "core/Geometry.h"
#include "core/Parallel.h"

#include <rawconv/error.hpp>
#include <rawconv/raw_reader.hpp>
#include <rawconv/renderer.hpp>

namespace imageio {

std::vector<std::string> RawReader::extensions() const
{
    // DNG, and the raw formats of the cameras most used; LibRaw reads many more, which
    // canRead() accepts too.
    return {
        "dng",
        "cr2",
        "cr3",
        "crw",
        "nef",
        "nrw",
        "arw",
        "srf",
        "sr2",
        "raf",
        "orf",
        "rw2",
        "pef",
        "srw",
        "3fr",
        "iiq",
        "x3f"};
}

bool RawReader::canRead(const std::filesystem::path &path) const
{
    return ImageReader::canRead(path) || rawconv::isRawFileName(path);
}

namespace {

/// The rendered raw in the working space, upright.
core::Image toImage(const rawconv::RenderedImage &rendered)
{
    core::Image image{static_cast<int>(rendered.width), static_cast<int>(rendered.height)};
    core::parallelFor(0, image.height, [&](int begin, int end) {
        const std::size_t from = static_cast<std::size_t>(begin) * image.width * 3;
        const std::size_t to = static_cast<std::size_t>(end) * image.width * 3;
        for (std::size_t i = from; i < to; ++i) {
            image.data[i] = rendered.data[i] / 65535.0f;
        }
    });
    return core::geometry::orient(image, rendered.orientation);
}

} // namespace

core::Image RawReader::read(const std::filesystem::path &path) const
{
    try {
        return toImage(
            rawconv::renderImage(rawconv::readRawFile(path), rawconv::ColorSpace::REC2020));
    } catch (const rawconv::Error &e) {
        throw Error{"Cannot read " + path.string() + ": " + e.what()};
    }
}

CameraShot readCameraShot(const std::filesystem::path &path)
{
    try {
        const rawconv::RawImage raw = rawconv::readRawFile(path);
        const rawconv::RenderedImage rendered
            = rawconv::renderImage(raw, rawconv::ColorSpace::REC2020);
        return CameraShot{toImage(rendered), raw.preview_jpeg, rendered.orientation};
    } catch (const rawconv::Error &e) {
        throw Error{"Cannot read " + path.string() + ": " + e.what()};
    }
}

} // namespace imageio
