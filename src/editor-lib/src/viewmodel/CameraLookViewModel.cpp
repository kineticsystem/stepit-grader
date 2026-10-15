// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/viewmodel/CameraLookViewModel.h"
#include "editor/CameraLookUtils.h"
#include "editor/ConcurrentUtils.h"
#include "editor/Document.h"
#include "imageio/CameraShotReader.h"

#include <QFileInfo>
#include <QLocale>

#include <set>
#include <string>

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

std::shared_ptr<ProgressMonitor> CameraLookViewModel::start(const QStringList &raws)
{
    if (isRunning() || raws.isEmpty()) {
        return nullptr;
    }
    m_status = std::make_shared<ProgressMonitor>();
    // A grade found before is not this match's: it is not offered once this one fails.
    m_result.reset();
    // The picture's settings as the match starts: the grade found goes over them.
    ConcurrentUtils::run(
        this,
        [raws,
         reader = m_reader,
         pipeline = m_pipeline,
         base = m_document->settings(),
         status = m_status] {
            return CameraLookUtils::matchCameraLook(raws, *reader, *pipeline, base, *status);
        },
        [this](const std::expected<core::match::Result, QString> &outcome) {
            m_status.reset();
            if (!outcome) {
                Q_EMIT failed(outcome.error());
                return;
            }
            m_result = *outcome;
            m_document->mergeSettings(m_result->settings);
            Q_EMIT finished(summary());
        });
    return m_status;
}

QString CameraLookViewModel::summary() const
{
    if (!m_result) {
        return {};
    }
    QString text
        = tr("Matched the camera's look in %1: the difference from its JPEG went from %2 to "
             "%3, after %4 trials.")
              .arg(QLocale{}.createSeparatedList(sectionsSet()))
              .arg(m_result->errorBefore, 0, 'f', 1)
              .arg(m_result->errorAfter, 0, 'f', 1)
              .arg(m_result->trials);
    if (m_result->stopped) {
        text += " " + tr("The search was stopped: this is the best grade found by then.");
    }
    return text;
}

QStringList CameraLookViewModel::sectionsSet() const
{
    std::set<std::string> ids;
    for (const core::match::Stage &stage : core::match::standardStages()) {
        ids.insert(stage.ids.begin(), stage.ids.end());
    }
    QStringList sections;
    for (const auto &effect : m_pipeline->effects()) {
        const QString section = QString::fromStdString(effect->section());
        for (const core::Parameter &parameter : effect->parameters()) {
            if (ids.contains(parameter.id) && !sections.contains(section)) {
                sections << section;
            }
        }
    }
    return sections;
}
