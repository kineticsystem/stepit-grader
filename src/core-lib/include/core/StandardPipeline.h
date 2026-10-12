// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Pipeline.h"

#include <memory>
#include <string>
#include <vector>

namespace core {

/**
 * The editor's effects, in the order they are applied: the white balance first, on the colours
 * as shot; the noise, before the tones brighten it and the sharpening sharpens it; the purple
 * fringes, on the hues the user sees, before the tones change the edges;
 * then the tones, global before local; then the colours; then the HDR compression, which brings
 * what the tones pushed above white back into the picture; the sharpening last, on the finished
 * image.
 */
std::shared_ptr<const Pipeline> makeStandardPipeline();

/**
 * The order the tools panel shows the sections of the standard pipeline in, which is not the order
 * the effects run in: the light first and the colour next, as raw editors show them, while the
 * white balance, in the colour, runs first.
 */
std::vector<std::string> standardSectionOrder();

} // namespace core
