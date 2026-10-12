// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <string>

namespace core {

/**
 * One value the user sets, e.g. the exposure: what it is called, its range and its default. The
 * id is the setting's name in the XMP sidecar's namespace, e.g. "Exposure2012", so that the
 * sidecar needs no table of its own.
 */
struct Parameter
{
    std::string id;
    std::string label;
    double minimum = -100;
    double maximum = 100;
    double defaultValue = 0;
    /// Decimals shown and saved: 0 for whole numbers.
    int decimals = 0;

    bool operator==(const Parameter &) const = default;
};

} // namespace core
