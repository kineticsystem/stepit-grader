// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Parallel.h"

#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <thread>

namespace {

/// Enough items for a range per thread, whatever the machine.
int manyItems()
{
    return 64 * static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
}

} // namespace

TEST(ParallelTest, CoversTheRangeOnce)
{
    const int count = manyItems();
    std::atomic<int> covered{0};
    std::atomic<long> sum{0};
    core::parallelFor(0, count, [&](int begin, int end) {
        for (int i = begin; i < end; ++i) {
            ++covered;
            sum += i;
        }
    });
    EXPECT_EQ(covered, count);
    EXPECT_EQ(sum, static_cast<long>(count) * (count - 1) / 2);
}

TEST(ParallelTest, ThrowsTheExceptionOfARangeOnceEveryRangeHasEnded)
{
    const int count = manyItems();
    // Thrown by the last range, which another thread runs when there is one.
    EXPECT_THROW(
        core::parallelFor(
            0,
            count,
            [count](int, int end) {
                if (end == count) {
                    throw std::runtime_error{"out of memory"};
                }
            }),
        std::runtime_error);
    // And by the first, which the calling thread may run itself.
    EXPECT_THROW(
        core::parallelFor(
            0,
            count,
            [](int begin, int) {
                if (begin == 0) {
                    throw std::runtime_error{"out of memory"};
                }
            }),
        std::runtime_error);

    // The threads are still there for the next call.
    std::atomic<int> covered{0};
    core::parallelFor(0, count, [&](int begin, int end) { covered += end - begin; });
    EXPECT_EQ(covered, count);
}

TEST(ParallelTest, ThrowsOnceWhenEveryRangeThrows)
{
    EXPECT_THROW(
        core::parallelFor(0, manyItems(), [](int, int) { throw std::runtime_error{"out of memory"}; }),
        std::runtime_error);
}
