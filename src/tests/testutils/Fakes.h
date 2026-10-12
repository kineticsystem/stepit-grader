// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

// Stand-ins for the reader and the store, for the parts that are given them.

#include "TestImages.h"

#include "imageio/Error.h"
#include "imageio/ImageReader.h"
#include "imageio/SettingsStore.h"

#include <map>
#include <mutex>

namespace testutils {

/// Gives a grey image for any name ending in .tif, and fails for the others.
class FakeReader : public imageio::ImageReader
{
public:
    std::vector<std::string> extensions() const override { return {"tif"}; }
    core::Image read(const std::filesystem::path &path) const override
    {
        if (path.extension() != ".tif") {
            throw imageio::Error{"cannot read " + path.string()};
        }
        return testutils::uniform(300, 200, 0.18f, 0.18f, 0.18f);
    }
};

/**
 * Keeps the settings in memory, per picture in `files`, which saving fills; a picture without its
 * own has `stored`, the settings of every picture.
 */
class FakeStore : public imageio::SettingsStore
{
public:
    std::filesystem::path location(const std::filesystem::path &image) const override
    {
        return std::filesystem::path{image}.replace_extension(".xmp");
    }
    std::optional<core::Settings> load(
        const std::filesystem::path &image, const std::vector<core::Parameter> &) const override
    {
        const std::lock_guard lock{mutex};
        const auto file = files.find(image);
        return file != files.end() ? std::optional{file->second} : stored;
    }
    void save(
        const std::filesystem::path &image,
        const core::Settings &settings,
        const std::vector<core::Parameter> &) const override
    {
        const std::lock_guard lock{mutex};
        if (failSaving) {
            throw imageio::Error{"disk full"};
        }
        files[image] = settings;
        ++saves;
    }

    mutable std::mutex mutex;
    mutable std::optional<core::Settings> stored;
    mutable std::map<std::filesystem::path, core::Settings> files;
    mutable int saves = 0;
    bool failSaving = false;
};

} // namespace testutils
