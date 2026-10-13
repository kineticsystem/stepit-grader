// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "imageio/Files.h"
#include "imageio/Error.h"

#include <atomic>
#include <chrono>
#include <string>

namespace imageio::files {

namespace {

/// The writes started by this program, so each has a temporary name of its own.
std::atomic<unsigned long> writes{0};

} // namespace

std::filesystem::path temporaryFor(const std::filesystem::path &path)
{
    // The clock tells apart two programs writing the same file; the counter, two writes of this one.
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path temporary = path;
    temporary.replace_filename(
        "." + path.filename().string() + "." + std::to_string(ticks) + "."
        + std::to_string(++writes) + ".part");
    return temporary;
}

void replace(const std::filesystem::path &temporary, const std::filesystem::path &path)
{
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (!error) {
        return;
    }
    std::error_code ignored;
    if (std::filesystem::is_regular_file(path, ignored)) {
        // Some systems do not rename over a file: the old one is set aside first, and brought
        // back if the new one cannot take its place.
        std::filesystem::path aside = temporaryFor(path);
        aside.replace_extension(".old");
        std::error_code setAside;
        std::filesystem::rename(path, aside, setAside);
        if (!setAside) {
            std::error_code put;
            std::filesystem::rename(temporary, path, put);
            if (!put) {
                std::filesystem::remove(aside, ignored);
                return;
            }
            std::filesystem::rename(aside, path, ignored);
        }
    }
    std::filesystem::remove(temporary, ignored);
    throw Error{"Cannot replace " + path.string() + ": " + error.message()};
}

} // namespace imageio::files
