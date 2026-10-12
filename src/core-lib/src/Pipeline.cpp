// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Pipeline.h"

namespace core {

Pipeline::Pipeline(std::vector<std::shared_ptr<const Effect>> effects)
    : m_effects{std::move(effects)}
{}

std::vector<Parameter> Pipeline::parameters() const
{
    std::vector<Parameter> all;
    for (const auto &effect : m_effects) {
        for (Parameter &parameter : effect->parameters()) {
            all.push_back(std::move(parameter));
        }
    }
    return all;
}

std::optional<Image> Pipeline::render(
    Image image,
    const Settings &settings,
    const RenderContext &context,
    const Cancelled &cancelled) const
{
    for (const auto &effect : m_effects) {
        if (cancelled && cancelled()) {
            return std::nullopt;
        }
        if (!effect->isIdentity(settings)) {
            effect->apply(image, settings, context);
        }
    }
    if (cancelled && cancelled()) {
        return std::nullopt;
    }
    return image;
}

} // namespace core
