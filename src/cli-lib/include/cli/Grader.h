// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "cli/Options.h"

#include "core/Pipeline.h"
#include "imageio/ImageReader.h"
#include "imageio/SettingsStore.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cli {

/// What became of one picture.
struct GradeResult
{
    std::filesystem::path input;
    std::filesystem::path output;
    bool graded = false;
    /// Why it was not, or a note when it was, e.g. that it had no sidecar.
    std::string message;
};

/**
 * Grades pictures with saved settings: reads each, applies the pipeline at full size, and writes
 * it as a TIFF. It is given its reader, its store and its pipeline, so the tests give it fakes.
 * One picture that fails is reported, and the others are graded.
 */
class Grader
{
public:
    using Progress
        = std::function<void(const GradeResult &result, std::size_t done, std::size_t total)>;

    Grader(
        std::shared_ptr<const imageio::ImageReader> reader,
        std::shared_ptr<const imageio::SettingsStore> store,
        std::shared_ptr<const core::Pipeline> pipeline);

    /**
     * Grades the inputs of the options. `settings` are those of --settings, read already; without
     * them, each picture's own sidecar is used.
     */
    std::vector<GradeResult> run(
        const Options &options,
        const std::optional<core::Settings> &settings,
        const Progress &progress = {}) const;

    /// Where a picture's TIFF goes: the output folder, the picture's name, .tif.
    static std::filesystem::path outputFor(
        const std::filesystem::path &input, const std::filesystem::path &folder);

private:
    GradeResult grade(
        const std::filesystem::path &input,
        const std::filesystem::path &output,
        const Options &options,
        const std::optional<core::Settings> &settings) const;

    std::shared_ptr<const imageio::ImageReader> m_reader;
    std::shared_ptr<const imageio::SettingsStore> m_store;
    std::shared_ptr<const core::Pipeline> m_pipeline;
};

} // namespace cli
