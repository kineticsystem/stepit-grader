// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/SaturationEffect.h"
#include "core/Color.h"
#include "core/Parallel.h"

namespace core {

void SaturationEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float factor = 1.0f + static_cast<float>(settings.value(SATURATION) / 100.0);
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                float *p = image.pixel(x, y);
                float lab[3];
                color::rec2020ToOklab(p, lab);
                lab[1] *= factor;
                lab[2] *= factor;
                color::oklabToRec2020(lab, p);
            }
        }
    });
}

} // namespace core
