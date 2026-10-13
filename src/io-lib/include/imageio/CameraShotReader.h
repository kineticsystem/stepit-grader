// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace imageio {

/// A raw picture and the JPEG its camera made of it.
struct CameraShot
{
    /// The picture as RawReader::read() gives it: upright, in linear Rec. 2020.
    core::Image image;
    /// The camera's JPEG, as the camera stored it, e.g. the one it shows on its screen; empty if
    /// the file has none.
    std::vector<std::uint8_t> cameraJpeg;
    /// The TIFF/EXIF orientation that turns the JPEG upright, as it turned the picture.
    int orientation = 1;
};

/**
 * Reads a picture with its camera's JPEG, for the camera match. An interface beside ImageReader,
 * so the match is given a fake in tests, and the editor knows which files hold such a JPEG
 * without naming a reader.
 */
class CameraShotReader
{
public:
    virtual ~CameraShotReader() = default;

    /// The extensions it reads, lower case, without the dot, e.g. "cr2".
    virtual std::vector<std::string> extensions() const = 0;

    /// Whether it reads the file, from its name.
    virtual bool canRead(const std::filesystem::path &path) const;

    /// Throws imageio::Error when the file cannot be read. Safe to call from any thread.
    virtual CameraShot read(const std::filesystem::path &path) const = 0;
};

} // namespace imageio
