// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/CameraMatch.h"
#include "core/Image.h"
#include "core/Pipeline.h"
#include "editor/Status.h"
#include "imageio/CameraShotReader.h"

#include <QImage>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <expected>
#include <vector>

/// The long side of the pictures the match compares, in pixels: enough for the tones and the
/// colours, small enough for a trial to take a few tens of milliseconds.
constexpr int CAMERA_LOOK_SIZE = 1000;

/**
 * A camera's JPEG as a picture to compare: decoded, in linear sRGB, turned upright by
 * `orientation` as the raw was, and of the raw's size, `width` × `height` once upright. A JPEG of
 * another shape than the raw's is refused: it is not the same picture.
 */
std::expected<core::Image, QString> cameraJpegImage(
    const std::vector<std::uint8_t> &jpeg, int orientation, int width, int height);

/**
 * Finds the grade that makes the raw files look like their camera's JPEGs: reads them with
 * `reader`, then searches the sliders with core::match::fit(), the others at their values in
 * `base`, the picture's settings. Reports how it goes to `status`, and stops when asked, with the
 * best grade found so far. Runs on any thread.
 */
std::expected<core::match::Result, QString> matchCameraLook(
    const QStringList &raws,
    const imageio::CameraShotReader &reader,
    const core::Pipeline &pipeline,
    const core::Settings &base,
    StatusController &status);
