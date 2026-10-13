// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "imageio/CameraShotReader.h"
#include "imageio/ImageReader.h"

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

/// Reads a camera raw file or a DNG with its camera's JPEG. Throws imageio::Error on failure.
CameraShot readCameraShot(const std::filesystem::path &path);

/// The camera shots of the files RawReader reads, through readCameraShot().
class RawShotReader : public CameraShotReader
{
public:
    std::vector<std::string> extensions() const override;
    bool canRead(const std::filesystem::path &path) const override;
    CameraShot read(const std::filesystem::path &path) const override;
};

} // namespace imageio
