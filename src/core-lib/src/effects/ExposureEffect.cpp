// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/ExposureEffect.h"
#include "core/Parallel.h"

#include <cmath>

namespace core {

void ExposureEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float gain = static_cast<float>(std::exp2(settings.value(EXPOSURE)));
    parallelForEach(image.data.size(), [&](std::size_t i) { image.data[i] *= gain; });
}

} // namespace core
