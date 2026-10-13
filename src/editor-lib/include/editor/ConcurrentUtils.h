// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QtConcurrent>

#include <type_traits>
#include <utility>

/**
 * Work done off the user interface's thread whose result comes back to it: the editor's view
 * models run their long work, e.g. saving the image or matching a camera's look, through run(),
 * rather than each keeping a `QFutureWatcher` of its own.
 */
namespace ConcurrentUtils {

/**
 * Runs `work` on Qt's global thread pool, then `done` with its result on the thread of `context`,
 * from its event loop. If `context` is destroyed first, `done` is not called; `work` still runs
 * to its end, so it holds copies, not references, of what it uses.
 */
template<typename Work, typename Done>
void run(QObject *context, Work work, Done done)
{
    using Result = std::invoke_result_t<Work>;
    auto *watcher = new QFutureWatcher<Result>{context};
    QObject::connect(
        watcher, &QFutureWatcherBase::finished, context, [watcher, done = std::move(done)] {
            watcher->deleteLater();
            if constexpr (std::is_void_v<Result>) {
                done();
            } else {
                done(watcher->result());
            }
        });
    watcher->setFuture(QtConcurrent::run(std::move(work)));
}

} // namespace ConcurrentUtils
