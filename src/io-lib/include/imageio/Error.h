// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <stdexcept>
#include <string>

namespace imageio {

/// Thrown when a file cannot be read or written; the message says which and why.
class Error : public std::runtime_error
{
public:
    explicit Error(const std::string &message)
        : std::runtime_error{message}
    {}
};

} // namespace imageio
