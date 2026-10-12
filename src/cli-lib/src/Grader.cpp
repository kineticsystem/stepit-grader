// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "cli/Grader.h"

#include "imageio/TiffWriter.h"

#include <set>

namespace cli {

namespace fs = std::filesystem;

namespace {

/// The same file, whether or not it exists yet.
bool sameFile(const fs::path &a, const fs::path &b)
{
    std::error_code error;
    if (fs::exists(a, error) && fs::exists(b, error)) {
        return fs::equivalent(a, b, error);
    }
    return fs::weakly_canonical(a, error) == fs::weakly_canonical(b, error);
}

} // namespace

Grader::Grader(
    std::shared_ptr<const imageio::ImageReader> reader,
    std::shared_ptr<const imageio::SettingsStore> store,
    std::shared_ptr<const core::Pipeline> pipeline)
    : m_reader{std::move(reader)}
    , m_store{std::move(store)}
    , m_pipeline{std::move(pipeline)}
{}

fs::path Grader::outputFor(const fs::path &input, const fs::path &folder)
{
    return folder / input.filename().replace_extension(".tif");
}

std::vector<GradeResult> Grader::run(
    const Options &options,
    const std::optional<core::Settings> &settings,
    const Progress &progress) const
{
    std::vector<GradeResult> results;
    std::error_code error;
    fs::create_directories(options.output, error);

    std::set<fs::path> written;
    for (const fs::path &input : options.inputs) {
        const fs::path output = outputFor(input, options.output);
        GradeResult result;
        if (written.contains(output)) {
            // E.g. a.CR2 and a.tif: the second would replace the first's TIFF.
            result = {input, output, false, "another picture of this run has the same name"};
        } else {
            result = grade(input, output, options, settings);
            if (result.graded) {
                written.insert(output);
            }
        }
        results.push_back(result);
        if (progress) {
            progress(result, results.size(), options.inputs.size());
        }
    }
    return results;
}

GradeResult Grader::grade(
    const fs::path &input,
    const fs::path &output,
    const Options &options,
    const std::optional<core::Settings> &settings) const
{
    GradeResult result{input, output, false, {}};
    if (sameFile(input, output)) {
        result.message = "the TIFF would replace the picture itself: choose another output folder";
        return result;
    }
    if (fs::exists(output) && !options.overwrite) {
        result.message = output.string() + " exists: give --overwrite to replace it";
        return result;
    }
    try {
        core::Settings used;
        if (settings) {
            used = *settings;
        } else if (auto own = m_store->load(input, m_pipeline->parameters())) {
            used = *own;
        } else {
            result.message = "no sidecar: graded with no edit";
        }
        const auto image = m_pipeline->render(m_reader->read(input), used, core::RenderContext{1.0});
        imageio::TiffWriter{{options.colourSpace, options.compress}}.write(*image, output);
        result.graded = true;
    } catch (const std::exception &e) {
        result.message = e.what();
    }
    return result;
}

} // namespace cli
