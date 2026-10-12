// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/Sections.h"

#include <algorithm>

std::vector<AdjustmentsPanel::Section> sectionsOf(
    const core::Pipeline &pipeline, const std::vector<std::string> &order)
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
    // Where a section stands in `order`; past its end for the sections it does not name, which
    // keep the order of the pipeline, the sort being stable.
    const auto rank = [&order](const AdjustmentsPanel::Section &section) {
        return std::ranges::find(order, section.title.toStdString()) - order.begin();
    };
    std::ranges::stable_sort(sections, {}, rank);
    return sections;
}
