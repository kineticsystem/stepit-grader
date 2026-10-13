// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/ConcurrentUtils.h"

#include <QTest>
#include <QThread>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace {

constexpr int WAIT_MS = 5000;

} // namespace

TEST(ConcurrentUtilsTest, RunsTheWorkOffThisThreadAndGivesItsResultBackOnIt)
{
    QObject context;
    QThread *workThread = nullptr;
    QThread *doneThread = nullptr;
    int result = 0;
    ConcurrentUtils::run(
        &context,
        [&workThread] {
            workThread = QThread::currentThread();
            return 42;
        },
        [&](int value) {
            doneThread = QThread::currentThread();
            result = value;
        });
    ASSERT_TRUE(QTest::qWaitFor([&] { return result != 0; }, WAIT_MS));
    EXPECT_EQ(result, 42);
    EXPECT_NE(workThread, QThread::currentThread());
    EXPECT_EQ(doneThread, QThread::currentThread());
}

TEST(ConcurrentUtilsTest, CallsNothingOnceItsContextIsGone)
{
    auto finished = std::make_shared<std::atomic<bool>>(false);
    bool called = false;
    {
        QObject context;
        ConcurrentUtils::run(
            &context,
            [finished] {
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
                *finished = true;
                return 1;
            },
            [&called](int) { called = true; });
    }
    ASSERT_TRUE(QTest::qWaitFor([&] { return finished->load(); }, WAIT_MS));
    QTest::qWait(50);
    EXPECT_FALSE(called);
}

TEST(ConcurrentUtilsTest, RunsWorkWithoutAResult)
{
    QObject context;
    std::atomic<bool> worked = false;
    bool done = false;
    ConcurrentUtils::run(&context, [&worked] { worked = true; }, [&done] { done = true; });
    ASSERT_TRUE(QTest::qWaitFor([&] { return done; }, WAIT_MS));
    EXPECT_TRUE(worked);
}
