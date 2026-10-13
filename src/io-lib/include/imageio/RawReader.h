// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "imageio/ImageReader.h"

#include <cstdint>
#include <vector>

namespace imageio {

/**
 * Reads camera raw files and DNGs with raw-converter: demosaiced, white balanced as shot, in
 * linear Rec. 2020, with nothing else applied. The sensor's clipping level is 1.0.
 */
class RawReader : public ImageReader
{
public:
    std::vector<std::string> extensions() const override;
    bool canRead(const std::filesystem::path &path) const override;
    core::Image read(const std::filesystem::path &path) const override;
};

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

/// Reads a camera raw file or a DNG with its camera's JPEG. Throws imageio::Error on failure.
CameraShot readCameraShot(const std::filesystem::path &path);

} // namespace imageio
