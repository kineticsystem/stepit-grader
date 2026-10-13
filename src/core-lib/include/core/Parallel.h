// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <functional>

namespace core {

/**
 * Calls body(begin, end) on consecutive ranges that cover [begin, end), one range per thread,
 * and returns once every range is done. Small ranges run on the calling thread.
 * An exception thrown by `body` is thrown here, once every range has ended; the ranges not started
 * by then are skipped.
 */
void parallelFor(int begin, int end, const std::function<void(int, int)> &body);

/// Calls body(i) for every i in [0, count), spread over the threads.
template<typename Body>
void parallelForEach(std::size_t count, Body body)
{
    parallelFor(0, static_cast<int>(count), [&body](int begin, int end) {
        for (int i = begin; i < end; ++i) {
            body(static_cast<std::size_t>(i));
        }
    });
}

} // namespace core
