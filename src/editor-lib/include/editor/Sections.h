// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Pipeline.h"
#include "editor/widgets/AdjustmentsPanel.h"

#include <vector>

/// The sections of the tools panel: one per section of the effects, in the order of the pipeline.
std::vector<AdjustmentsPanel::Section> sectionsOf(const core::Pipeline &pipeline);
