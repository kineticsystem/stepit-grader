// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/viewmodel/CameraLookViewModel.h"
#include "editor/CameraLook.h"
#include "editor/Document.h"
#include "imageio/CameraShotReader.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent>

CameraLookViewModel::CameraLookViewModel(
    Document *document,
    std::shared_ptr<const core::Pipeline> pipeline,
    std::shared_ptr<const imageio::CameraShotReader> reader,
    QObject *parent)
    : QObject{parent}
    , m_document{document}
    , m_pipeline{std::move(pipeline)}
    , m_reader{std::move(reader)}
{}

bool CameraLookViewModel::matchesOpenPicture() const
{
    return m_document->isOpen()
           && m_reader->canRead(std::filesystem::path{m_document->path().toStdU16String()});
}

QString CameraLookViewModel::picture() const
{
    return m_document->path();
}

QString CameraLookViewModel::rawNameFilter() const
{
    QStringList patterns;
    for (const std::string &extension : m_reader->extensions()) {
        const QString suffix = QString::fromStdString(extension);
        patterns << "*." + suffix << "*." + suffix.toUpper();
    }
    return tr("Camera raw files (%1)").arg(patterns.join(' '));
}

QString CameraLookViewModel::folder() const
{
    return QFileInfo{m_document->path()}.absolutePath();
}

std::shared_ptr<StatusController> CameraLookViewModel::start(const QStringList &raws)
{
    if (isRunning() || raws.isEmpty()) {
        return nullptr;
    }
    m_status = std::make_shared<StatusController>();
    using Outcome = std::expected<core::match::Result, QString>;
    auto *watcher = new QFutureWatcher<Outcome>{this};
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        watcher->deleteLater();
        m_status.reset();
        const Outcome outcome = watcher->result();
        if (!outcome) {
            Q_EMIT failed(outcome.error());
            return;
        }
        m_result = *outcome;
        Q_EMIT finished();
    });
    // The picture's settings as the match starts: the grade found goes over them.
    watcher->setFuture(QtConcurrent::run(
        [raws,
         reader = m_reader,
         pipeline = m_pipeline,
         base = m_document->settings(),
         status = m_status] { return matchCameraLook(raws, *reader, *pipeline, base, *status); }));
    return m_status;
}

QString CameraLookViewModel::summary() const
{
    if (!m_result) {
        return {};
    }
    QString text = tr("The difference from the camera's JPEG is %1 as the picture is now, and %2 "
                      "with the grade found, after %3 trials.")
                       .arg(m_result->errorBefore, 0, 'f', 1)
                       .arg(m_result->errorAfter, 0, 'f', 1)
                       .arg(m_result->trials);
    if (m_result->stopped) {
        text += "\n\n" + tr("The search was stopped: this is the best grade found by then.");
    }
    return text;
}

void CameraLookViewModel::apply()
{
    if (m_result) {
        m_document->mergeSettings(m_result->settings);
    }
}
