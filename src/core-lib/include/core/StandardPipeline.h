// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Pipeline.h"

#include <memory>

namespace core {

/**
 * The editor's effects, in the order they are applied: the white balance first, on the colours
 * as shot; the purple fringes, on the hues the user sees, before the tones change the edges;
 * then the tones, global before local; then the colours; then the HDR compression, which brings
 * what the tones pushed above white back into the picture; the sharpening last, on the finished
 * image. The order of the tools panel is the same.
 */
std::shared_ptr<const Pipeline> makeStandardPipeline();

} // namespace core
