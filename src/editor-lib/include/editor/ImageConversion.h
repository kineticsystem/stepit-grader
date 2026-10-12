// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

#include <QImage>

/// The image for the screen: 8-bit sRGB, see core::display::toSrgb8().
QImage toQImage(const core::Image &image);
