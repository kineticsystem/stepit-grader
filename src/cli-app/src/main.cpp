// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "cli/Grader.h"
#include "cli/Options.h"

#include "core/StandardPipeline.h"
#include "imageio/ImageLoader.h"
#include "imageio/XmpSidecar.h"

#include <cstdio>
#include <iostream>

#ifdef __GLIBC__
#include <malloc.h>
#endif

/// The command line's composition root: grades pictures with saved settings, without a window.
int main(int argc, char *argv[])
{
#ifdef __GLIBC__
    // As in the editor: the effects' planes are reused rather than given back to the kernel.
    mallopt(M_MMAP_THRESHOLD, 1 << 30);
    mallopt(M_TRIM_THRESHOLD, 1 << 30);
#endif
    const std::string program = "stepit-grader-cli";
    const auto options = cli::parseOptions({argv + 1, argv + argc});
    if (!options) {
        std::cerr << program << ": " << options.error() << "\n"
                  << "Try '" << program << " --help'.\n";
        return 2;
    }
    if (options->help) {
        std::cout << cli::usage(program);
        return 0;
    }
    if (options->version) {
        std::cout << "StepIt Grader " << STEPIT_GRADER_VERSION << "\n";
        return 0;
    }

    const auto pipeline = core::makeStandardPipeline();
    std::optional<core::Settings> settings;
    if (options->settings) {
        try {
            settings = imageio::XmpSidecar::loadFile(*options->settings, pipeline->parameters());
        } catch (const std::exception &e) {
            std::cerr << program << ": " << e.what() << "\n";
            return 2;
        }
    }

    const cli::Grader grader{
        imageio::makeStandardImageReader(), std::make_shared<imageio::XmpSidecar>(), pipeline};
    const auto results = grader.run(
        *options, settings, [](const cli::GradeResult &result, std::size_t done, std::size_t total) {
            if (result.graded) {
                std::cout << "[" << done << "/" << total << "] " << result.output.string();
                if (!result.message.empty()) {
                    std::cout << " (" << result.message << ")";
                }
                std::cout << std::endl;
            } else {
                std::cerr << "[" << done << "/" << total << "] " << result.input.string() << ": "
                          << result.message << std::endl;
            }
        });

    std::size_t failed = 0;
    for (const auto &result : results) {
        failed += result.graded ? 0 : 1;
    }
    if (failed > 0) {
        std::cerr << program << ": " << failed << " of " << results.size() << " not graded\n";
        return 1;
    }
    return 0;
}
