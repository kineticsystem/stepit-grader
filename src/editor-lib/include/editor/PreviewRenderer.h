// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "core/Pipeline.h"
#include "core/Settings.h"

#include <QFuture>
#include <QImage>
#include <QList>
#include <QObject>
#include <QTimer>

#include <atomic>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/**
 * Renders the image with the settings, off the user interface's thread, in two passes:
 *
 * 1. At once, a preview: the image reduced so its longer side is at most `previewSide`, quick
 *    enough to follow a slider.
 * 2. Once the settings have not changed for `fullDelay`, the full image, for the detail.
 *
 * Each pass ends with rendered(). A change of the settings drops the passes the new settings make
 * useless: only the latest settings are rendered. A pass made old by a change stops at its next
 * effect; the preview, at most once between two pictures shown, so a slider dragged on and on
 * still shows a picture as often as a preview can be made.
 *
 * The last two pictures rendered in full are kept: settings rendered already, e.g. switching
 * between the picture before and after its edit, are shown at once, in full, without a pass.
 */
class PreviewRenderer : public QObject
{
    Q_OBJECT

public:
    explicit PreviewRenderer(
        std::shared_ptr<const core::Pipeline> pipeline,
        int previewSide = 2048,
        std::chrono::milliseconds fullDelay = std::chrono::milliseconds{300},
        QObject *parent = nullptr);
    ~PreviewRenderer() override;

    /// The image to render, in full; the preview is reduced from it by the first pass, off the thread.
    void setSource(std::shared_ptr<const core::Image> image);

public Q_SLOTS:
    void render(const core::Settings &settings);

    /// The pipeline it renders with, for other work that must grade as the preview does.
    std::shared_ptr<const core::Pipeline> pipeline() const { return m_pipeline; }

Q_SIGNALS:
    /// A pass is done: `image` stands for the whole picture, of `fullSize`; `full` when it is.
    void rendered(const QImage &image, QSize fullSize, bool full);

private:
    void startPreview();
    void startFull();
    /// Stops the preview running, if any, at its next effect.
    void stopPreview();
    void emitRendered(const QImage &image, QSize fullSize, bool full);
    /// Keeps a full picture, forgetting the oldest beyond the number kept.
    void remember(const core::Settings &settings, const QImage &image);

    std::shared_ptr<const core::Pipeline> m_pipeline;
    int m_previewSide;
    QTimer m_fullTimer;

    std::shared_ptr<const core::Image> m_full;
    /// Null until the first preview pass of the source has made it.
    std::shared_ptr<const core::Image> m_preview;

    core::Settings m_settings;
    /// Counts the changes; a pass whose number is no longer the latest stops.
    std::shared_ptr<std::atomic<int>> m_generation = std::make_shared<std::atomic<int>>(0);
    bool m_previewRunning = false;
    bool m_previewPending = false;
    /// Set to stop the preview running at its next effect.
    std::shared_ptr<std::atomic<bool>> m_stopPreview = std::make_shared<std::atomic<bool>>(false);
    /// A preview was stopped since the last picture shown: the next one runs to its end.
    bool m_previewStopped = false;
    /// The last pictures rendered in full, the latest last, with their settings.
    std::vector<std::pair<core::Settings, QImage>> m_rendered;

    QFuture<void> m_previewJob;
    /// The full passes still running, the stale ones included: waited for on destruction.
    QList<QFuture<void>> m_fullJobs;
};
