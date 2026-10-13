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
#include <set>
#include <thread>

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
        if (damaged.contains(image)) {
            throw imageio::Error{
                "Cannot read " + location(image).string() + ": not an XMP document"};
        }
        const auto file = files.find(image);
        return file != files.end() ? std::optional{file->second} : stored;
    }
    /// The settings saved for `file`, as an image's, or an error.
    core::Settings loadFile(
        const std::filesystem::path &file, const std::vector<core::Parameter> &) const override
    {
        const std::lock_guard lock{mutex};
        lastThread = std::this_thread::get_id();
        const auto found = files.find(file);
        if (found == files.end()) {
            throw imageio::Error{"Cannot read " + file.string() + ": no such file"};
        }
        return found->second;
    }
    std::string extension() const override { return "xmp"; }
    void save(
        const std::filesystem::path &image,
        const core::Settings &settings,
        const std::vector<core::Parameter> &) const override
    {
        const std::lock_guard lock{mutex};
        lastThread = std::this_thread::get_id();
        if (failSaving) {
            throw imageio::Error{"disk full"};
        }
        files[image] = settings;
        ++saves;
    }

    mutable std::mutex mutex;
    mutable std::optional<core::Settings> stored;
    mutable std::map<std::filesystem::path, core::Settings> files;
    /// The pictures whose settings cannot be read.
    std::set<std::filesystem::path> damaged;
    mutable int saves = 0;
    /// The thread of the last save or loadFile(), to check it is not the caller's.
    mutable std::thread::id lastThread;
    bool failSaving = false;
};

} // namespace testutils
