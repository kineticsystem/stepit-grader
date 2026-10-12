// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Parameter.h"
#include "core/Settings.h"

#include <filesystem>
#include <optional>
#include <vector>

namespace imageio {

/// Where the settings of an image are kept, and how. The editor sees only this interface.
class SettingsStore
{
public:
    virtual ~SettingsStore() = default;

    /// The file that holds the settings of the image.
    virtual std::filesystem::path location(const std::filesystem::path &image) const = 0;

    /**
     * The settings of the image, for the given parameters; nothing when it has none. Throws
     * imageio::Error when they exist but cannot be read.
     */
    virtual std::optional<core::Settings> load(
        const std::filesystem::path &image, const std::vector<core::Parameter> &parameters) const
        = 0;

    /// Saves every parameter's value. Throws imageio::Error when they cannot be written.
    virtual void save(
        const std::filesystem::path &image,
        const core::Settings &settings,
        const std::vector<core::Parameter> &parameters) const
        = 0;
};

} // namespace imageio
