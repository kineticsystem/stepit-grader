// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/ImageConversionUtils.h"

#include "core/Display.h"

#include <QColorSpace>

#include <cstring>

namespace ImageConversionUtils {

QImage toQImage(const core::Image &image)
{
    if (image.isNull()) {
        return {};
    }
    const std::vector<std::uint8_t> bytes = core::display::toSrgb8(image);
    QImage out{image.width, image.height, QImage::Format_RGB888};
    const std::size_t row = static_cast<std::size_t>(image.width) * 3;
    for (int y = 0; y < image.height; ++y) {
        std::memcpy(out.scanLine(y), &bytes[row * y], row);
    }
    out.setColorSpace(QColorSpace::SRgb);
    return out;
}

} // namespace ImageConversionUtils
