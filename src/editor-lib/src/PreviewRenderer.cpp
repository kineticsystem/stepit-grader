// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/PreviewRenderer.h"
#include "editor/ImageConversionUtils.h"

#include "core/Geometry.h"

#include <QPointer>
#include <QtConcurrent>

PreviewRenderer::PreviewRenderer(
    std::shared_ptr<const core::Pipeline> pipeline,
    int previewSide,
    std::chrono::milliseconds fullDelay,
    QObject *parent)
    : QObject{parent}
    , m_pipeline{std::move(pipeline)}
    , m_previewSide{previewSide}
{
    m_fullTimer.setSingleShot(true);
    m_fullTimer.setInterval(fullDelay);
    connect(&m_fullTimer, &QTimer::timeout, this, &PreviewRenderer::startFull);
}

PreviewRenderer::~PreviewRenderer()
{
    ++*m_generation;
    m_stopPreview->store(true);
    m_previewJob.waitForFinished();
    for (QFuture<void> &job : m_fullJobs) {
        job.waitForFinished();
    }
}

namespace {

/// The full pictures kept: the picture before its edit, and after.
constexpr std::size_t KEPT_RENDERS = 2;

} // namespace

void PreviewRenderer::setSource(std::shared_ptr<const core::Image> image)
{
    ++*m_generation;
    // A preview of another picture is of no use.
    stopPreview();
    m_fullTimer.stop();
    m_rendered.clear();
    m_full = std::move(image);
    // Reducing a picture of tens of megapixels takes a while: the first pass does it.
    m_preview.reset();
}

void PreviewRenderer::render(const core::Settings &settings)
{
    if (!m_full) {
        return;
    }
    m_settings = settings;
    ++*m_generation;
    m_fullTimer.stop();
    const auto parameters = m_pipeline->parameters();
    for (const auto &[kept, image] : m_rendered) {
        if (kept.sameAs(settings, parameters)) {
            // Rendered already: shown in full at once. A preview still running is of no use.
            m_previewPending = false;
            stopPreview();
            emitRendered(image, image.size(), true);
            return;
        }
    }
    if (m_previewRunning) {
        m_previewPending = true;
        // The preview running is for older settings: stopped, so the latest start sooner. Only
        // once until a picture is shown, or a slider dragged on would never show one.
        if (!m_previewStopped) {
            m_previewStopped = true;
            stopPreview();
        }
    } else {
        startPreview();
    }
}

void PreviewRenderer::startPreview()
{
    m_previewRunning = true;
    m_previewPending = false;
    m_stopPreview = std::make_shared<std::atomic<bool>>(false);
    const int generation = *m_generation;
    QPointer<PreviewRenderer> self{this};
    m_previewJob = QtConcurrent::run([self,
                                      pipeline = m_pipeline,
                                      source = m_preview,
                                      full = m_full,
                                      side = m_previewSide,
                                      settings = m_settings,
                                      counter = m_generation,
                                      stop = m_stopPreview,
                                      generation] {
        const auto preview = source ? source
                                    : std::make_shared<const core::Image>(
                                          core::geometry::downscale(*full, side));
        const double scale = static_cast<double>(preview->width) / full->width;
        const auto image = pipeline->render(*preview, settings, core::RenderContext{scale}, [stop] {
            return stop->load();
        });
        QImage shown = image ? ImageConversionUtils::toQImage(*image) : QImage{};
        const QSize fullSize{full->width, full->height};
        QMetaObject::invokeMethod(
            qApp,
            [self, preview, full, shown = std::move(shown), fullSize, counter, generation] {
                if (!self) {
                    return;
                }
                self->m_previewRunning = false;
                // Kept for the next passes, unless another picture came meanwhile.
                if (self->m_full == full && !self->m_preview) {
                    self->m_preview = preview;
                }
                // A preview of older settings is still shown while a newer one is made: better
                // than nothing. One whose settings were shown in full meanwhile is not.
                if (!shown.isNull() && (*counter == generation || self->m_previewPending)) {
                    self->emitRendered(shown, fullSize, false);
                }
                if (self->m_previewPending) {
                    self->startPreview();
                } else if (*counter == generation) {
                    self->m_fullTimer.start();
                }
            },
            Qt::QueuedConnection);
    });
}

void PreviewRenderer::startFull()
{
    if (!m_full) {
        return;
    }
    // A full pass of older settings may still be running: it sees it is stale and stops at its
    // next effect, without being waited for here.
    m_fullJobs.removeIf([](const QFuture<void> &job) { return job.isFinished(); });
    const int generation = *m_generation;
    QPointer<PreviewRenderer> self{this};
    m_fullJobs << QtConcurrent::run([self,
                                     pipeline = m_pipeline,
                                     source = m_full,
                                     settings = m_settings,
                                     counter = m_generation,
                                     generation] {
        const auto stale = [counter, generation] { return *counter != generation; };
        const auto image = pipeline->render(*source, settings, core::RenderContext{1.0}, stale);
        if (!image || stale()) {
            return;
        }
        QImage shown = ImageConversionUtils::toQImage(*image);
        QMetaObject::invokeMethod(
            qApp,
            [self, settings, shown = std::move(shown), counter, generation] {
                if (self && *counter == generation) {
                    self->remember(settings, shown);
                    self->emitRendered(shown, shown.size(), true);
                }
            },
            Qt::QueuedConnection);
    });
}

void PreviewRenderer::stopPreview()
{
    m_stopPreview->store(true);
}

void PreviewRenderer::emitRendered(const QImage &image, QSize fullSize, bool full)
{
    // A picture shown: the next change may stop a preview again.
    m_previewStopped = false;
    Q_EMIT rendered(image, fullSize, full);
}

void PreviewRenderer::remember(const core::Settings &settings, const QImage &image)
{
    m_rendered.emplace_back(settings, image);
    if (m_rendered.size() > KEPT_RENDERS) {
        m_rendered.erase(m_rendered.begin());
    }
}
