// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Parallel.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace core {

namespace {

/// Below this many items, sharing out the work costs more than it saves.
constexpr int MIN_ITEMS_PER_CHUNK = 16;

/**
 * Threads started once and kept, as an effect calls parallelFor about ten times and starting
 * twenty threads each time would cost more than a small image's work. Any thread may submit work,
 * and waits by running the work queued, so two renders at once share the threads, and a
 * parallelFor inside another cannot deadlock.
 */
class ThreadPool
{
public:
    ThreadPool()
    {
        const unsigned count = std::max(1u, std::thread::hardware_concurrency()) - 1;
        for (unsigned i = 0; i < count; ++i) {
            m_workers.emplace_back([this](std::stop_token stop) { work(stop); });
        }
    }

    ~ThreadPool()
    {
        for (auto &worker : m_workers) {
            worker.request_stop();
        }
        m_wake.notify_all();
    }

    int threads() const { return static_cast<int>(m_workers.size()) + 1; }

    void run(std::vector<std::function<void()>> chunks)
    {
        Group group;
        group.remaining = static_cast<int>(chunks.size());
        {
            const std::lock_guard lock{m_mutex};
            for (auto &chunk : chunks) {
                m_queue.push_back([&group, chunk = std::move(chunk)] {
                    chunk();
                    // Counted and signalled under the group's lock: the submitter returns, and
                    // the group on its stack goes, only once it holds that lock after the last
                    // chunk has let it go.
                    const std::lock_guard doneLock{group.mutex};
                    if (--group.remaining == 0) {
                        group.done.notify_all();
                    }
                });
            }
        }
        m_wake.notify_all();
        // Helps with what is queued, then waits for the chunks the other threads still run.
        while (runOne()) {
        }
        std::unique_lock lock{group.mutex};
        group.done.wait(lock, [&group] { return group.remaining == 0; });
    }

private:
    struct Group
    {
        int remaining = 0;
        std::mutex mutex;
        std::condition_variable done;
    };

    bool runOne()
    {
        std::function<void()> task;
        {
            const std::lock_guard lock{m_mutex};
            if (m_queue.empty()) {
                return false;
            }
            task = std::move(m_queue.front());
            m_queue.pop_front();
        }
        task();
        return true;
    }

    void work(std::stop_token stop)
    {
        while (!stop.stop_requested()) {
            std::function<void()> task;
            {
                std::unique_lock lock{m_mutex};
                m_wake.wait(lock, stop, [this] { return !m_queue.empty(); });
                if (m_queue.empty()) {
                    continue;
                }
                task = std::move(m_queue.front());
                m_queue.pop_front();
            }
            task();
        }
    }

    std::mutex m_mutex;
    std::condition_variable_any m_wake;
    std::deque<std::function<void()>> m_queue;
    std::vector<std::jthread> m_workers;
};

ThreadPool &pool()
{
    static ThreadPool instance;
    return instance;
}

} // namespace

void parallelFor(int begin, int end, const std::function<void(int, int)> &body)
{
    const int count = end - begin;
    if (count <= 0) {
        return;
    }
    const int chunks = std::clamp(count / MIN_ITEMS_PER_CHUNK, 1, pool().threads());
    if (chunks == 1) {
        body(begin, end);
        return;
    }
    const int size = (count + chunks - 1) / chunks;
    std::vector<std::function<void()>> work;
    for (int start = begin; start < end; start += size) {
        work.emplace_back([&body, start, stop = std::min(end, start + size)] { body(start, stop); });
    }
    pool().run(std::move(work));
}

} // namespace core
