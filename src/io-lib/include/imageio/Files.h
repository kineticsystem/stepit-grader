// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>

/**
 * Writing a file without ever harming the one it replaces: the new one is written whole to a
 * temporary file of its own beside it, then put in its place. The TIFF writer, the sidecar and the
 * editor's JPEG save write through these.
 */
namespace imageio::files {

/**
 * A name beside `path` to write it to first, hidden and unique to this call, e.g.
 * `.IMG_0001.tif.3.part`: two writes of the same file at once never share one.
 */
std::filesystem::path temporaryFor(const std::filesystem::path &path);

/**
 * Puts `temporary` in place of `path`, which may exist, and removes `temporary` if it cannot.
 * Where the system does not rename over a file, the old one is set aside first, and brought back
 * if the new one cannot take its place: `path` is the old file or the new one, never neither.
 * Throws imageio::Error when `path` is not replaced.
 */
void replace(const std::filesystem::path &temporary, const std::filesystem::path &path);

} // namespace imageio::files
