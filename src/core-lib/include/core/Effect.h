// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "core/Parameter.h"
#include "core/Settings.h"

#include <string>
#include <vector>

namespace core {

/// What an effect needs to know besides the image and the settings.
struct RenderContext
{
    /**
     * The size of the image rendered against the full image, e.g. 0.33 for a preview a third as
     * wide. A radius in pixels of the full image is multiplied by it, so the preview looks as the
     * full image does.
     */
    double scale = 1.0;
};

/**
 * One adjustment of the picture, e.g. the exposure. It owns its parameters and how it changes the
 * image, and nothing else: it does not know the other effects, the order they run in, the user
 * interface or the files. An effect at its defaults leaves the image exactly as it is.
 */
class Effect
{
public:
    virtual ~Effect() = default;

    /// The section of the tools panel it belongs to, e.g. "Light".
    virtual std::string section() const = 0;

    virtual std::vector<Parameter> parameters() const = 0;

    /// Whether `apply` would leave the image as it is; by default, when every parameter is at its
    /// default.
    virtual bool isIdentity(const Settings &settings) const;

    /// Changes the image in place. Must be safe to call from several threads at once.
    virtual void apply(Image &image, const Settings &settings, const RenderContext &context) const
        = 0;
};

} // namespace core
