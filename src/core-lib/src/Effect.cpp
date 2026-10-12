// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Effect.h"

namespace core {

bool Effect::isIdentity(const Settings &settings) const
{
    for (const Parameter &parameter : parameters()) {
        if (settings.value(parameter) != parameter.defaultValue) {
            return false;
        }
    }
    return true;
}

} // namespace core
