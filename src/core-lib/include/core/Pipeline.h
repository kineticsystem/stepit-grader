// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace core {

/**
 * The effects, in the order they change the image. It knows only the Effect interface: which
 * effects there are is decided by whoever builds it, e.g. makeStandardPipeline().
 */
class Pipeline
{
public:
    using Cancelled = std::function<bool()>;

    explicit Pipeline(std::vector<std::shared_ptr<const Effect>> effects);

    const std::vector<std::shared_ptr<const Effect>> &effects() const { return m_effects; }

    /// Every parameter of every effect, in order.
    std::vector<Parameter> parameters() const;

    /**
     * The image with every effect applied, skipping those at their defaults. `cancelled` is asked
     * before each effect: once it says yes, the render stops and returns nothing.
     */
    std::optional<Image> render(
        Image image,
        const Settings &settings,
        const RenderContext &context = {},
        const Cancelled &cancelled = {}) const;

private:
    std::vector<std::shared_ptr<const Effect>> m_effects;
};

} // namespace core
