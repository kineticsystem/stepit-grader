// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/Sections.h"

std::vector<AdjustmentsPanel::Section> sectionsOf(const core::Pipeline &pipeline)
{
    std::vector<AdjustmentsPanel::Section> sections;
    for (const auto &effect : pipeline.effects()) {
        const QString title = QString::fromStdString(effect->section());
        auto it = std::ranges::find(sections, title, &AdjustmentsPanel::Section::title);
        if (it == sections.end()) {
            sections.push_back({title, {}});
            it = std::prev(sections.end());
        }
        for (const core::Parameter &parameter : effect->parameters()) {
            it->parameters.push_back(parameter);
        }
    }
    return sections;
}
