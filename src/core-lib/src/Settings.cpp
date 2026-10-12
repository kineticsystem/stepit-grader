// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Settings.h"

namespace core {

double Settings::value(const Parameter &parameter) const
{
    const auto it = m_values.find(parameter.id);
    return it == m_values.end() ? parameter.defaultValue : it->second;
}

void Settings::setValue(const std::string &id, double value)
{
    m_values[id] = value;
}

bool Settings::has(const std::string &id) const
{
    return m_values.contains(id);
}

bool Settings::sameAs(const Settings &other, const std::vector<Parameter> &parameters) const
{
    for (const Parameter &parameter : parameters) {
        if (value(parameter) != other.value(parameter)) {
            return false;
        }
    }
    return true;
}

} // namespace core
