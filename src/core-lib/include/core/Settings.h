// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Parameter.h"

#include <map>
#include <string>
#include <vector>

namespace core {

/**
 * The values of the parameters, by id. A parameter without a value has its default, so a new
 * Settings changes nothing, and a parameter added later reads as its default from old files.
 */
class Settings
{
public:
    double value(const Parameter &parameter) const;
    void setValue(const std::string &id, double value);
    bool has(const std::string &id) const;
    const std::map<std::string, double> &values() const { return m_values; }

    /// Whether the two give every parameter of the list the same value.
    bool sameAs(const Settings &other, const std::vector<Parameter> &parameters) const;

    bool operator==(const Settings &) const = default;

private:
    std::map<std::string, double> m_values;
};

} // namespace core
