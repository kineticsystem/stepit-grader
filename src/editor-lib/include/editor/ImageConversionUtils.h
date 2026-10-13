// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

#include <QImage>

/**
 * Converting the engine's images, linear Rec. 2020 in float, to Qt's, to show or to save.
 */
namespace ImageConversionUtils {

/// The image for the screen: 8-bit sRGB, see core::display::toSrgb8().
QImage toQImage(const core::Image &image);

} // namespace ImageConversionUtils
