// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Fits a scene brighter than white into the picture, instead of clipping it: an HDR tone
 * compression. The tones above the compression point roll off smoothly, so that up to 4 stops
 * above white end at white; the tones below are left as they are. The curve works on each pixel's
 * brightest channel, so the hues stay and no channel clips. With local contrast, it compresses
 * the brightness of whole areas, measured with an edge-preserving filter, and keeps the texture
 * within them, e.g. the clouds of a bright sky.
 *
 * The ids are the editor's own: other raw editors have no such setting.
 */
class ToneCompressionEffect : public Effect
{
public:
    static inline const Parameter AMOUNT{"HDRToneCompression", "HDR Compression", 0, 100, 0, 0};
    static inline const Parameter POINT{"HDRCompressionPoint", "Compression Point", 0, 100, 50, 0};
    static inline const Parameter LOCAL{"HDRLocalContrast", "Local Contrast", 0, 100, 50, 0};

    std::string section() const override { return "HDR"; }
    std::vector<Parameter> parameters() const override { return {AMOUNT, POINT, LOCAL}; }
    /// Nothing to do without an amount, whatever the point and the local contrast.
    bool isIdentity(const Settings &settings) const override;
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;

    /**
     * The curve: `value`, linear, with the tones above `knee` rolled off so that `white` ends at
     * 1. The slope is 1 at the knee, so the curve has no visible start. Exposed for the tests.
     */
    static float compress(float value, float knee, float white);
};

} // namespace core
