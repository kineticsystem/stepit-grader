// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/StandardPipeline.h"
#include "core/effects/BlacksEffect.h"
#include "core/effects/ColorNoiseEffect.h"
#include "core/effects/ContrastEffect.h"
#include "core/effects/DefringeEffect.h"
#include "core/effects/DetailEffect.h"
#include "core/effects/ExposureEffect.h"
#include "core/effects/HighlightsEffect.h"
#include "core/effects/LuminanceNoiseEffect.h"
#include "core/effects/SaturationEffect.h"
#include "core/effects/ShadowsEffect.h"
#include "core/effects/SharpnessEffect.h"
#include "core/effects/StructureEffect.h"
#include "core/effects/ToneCompressionEffect.h"
#include "core/effects/VibranceEffect.h"
#include "core/effects/WhiteBalanceEffect.h"

namespace core {

std::shared_ptr<const Pipeline> makeStandardPipeline()
{
    return std::make_shared<const Pipeline>(std::vector<std::shared_ptr<const Effect>>{
        std::make_shared<WhiteBalanceEffect>(),
        std::make_shared<LuminanceNoiseEffect>(),
        std::make_shared<ColorNoiseEffect>(),
        std::make_shared<DefringeEffect>(),
        std::make_shared<ExposureEffect>(),
        std::make_shared<ContrastEffect>(),
        std::make_shared<HighlightsEffect>(),
        std::make_shared<ShadowsEffect>(),
        std::make_shared<BlacksEffect>(),
        std::make_shared<DetailEffect>(),
        std::make_shared<StructureEffect>(),
        std::make_shared<VibranceEffect>(),
        std::make_shared<SaturationEffect>(),
        std::make_shared<ToneCompressionEffect>(),
        std::make_shared<SharpnessEffect>(),
    });
}

std::vector<std::string> standardSectionOrder()
{
    return {"Light", "Color", "Effects", "Noise Reduction", "Defringe", "HDR", "Sharpening"};
}

} // namespace core
