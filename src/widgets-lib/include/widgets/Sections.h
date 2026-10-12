// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Pipeline.h"
#include "widgets/AdjustmentsPanel.h"

#include <string>
#include <vector>

/**
 * The sections of the tools panel: one per section of the effects, each with its parameters in the
 * order of the pipeline. The sections named in `order` come first, in that order; the others
 * follow, in the order of the pipeline.
 */
std::vector<AdjustmentsPanel::Section> sectionsOf(
    const core::Pipeline &pipeline, const std::vector<std::string> &order = {});
