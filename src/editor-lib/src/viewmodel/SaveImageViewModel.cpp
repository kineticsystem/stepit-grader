// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/viewmodel/SaveImageViewModel.h"
#include "editor/ConcurrentUtils.h"
#include "editor/Document.h"
#include "editor/ImageExportUtils.h"
#include "editor/SessionProperties.h"

#include <QFileInfo>
#include <QSettings>

#include <algorithm>

namespace {

/// The format of `filter`, or the first for a filter that names none.
const ImageExportUtils::ImageFormat &formatOf(const QString &filter)
{
    const auto &formats = ImageExportUtils::imageFormats();
    const auto found = std::ranges::find(formats, filter, &ImageExportUtils::ImageFormat::filter);
    return found != formats.end() ? *found : formats.front();
}

} // namespace

SaveImageViewModel::SaveImageViewModel(
    Document *document,
    std::shared_ptr<const core::Pipeline> pipeline,
    QSettings *settings,
    QObject *parent)
    : QObject{parent}
    , m_document{document}
    , m_pipeline{std::move(pipeline)}
    , m_settings{settings}
{}

QStringList SaveImageViewModel::filters() const
{
    QStringList filters;
    for (const ImageExportUtils::ImageFormat &format : ImageExportUtils::imageFormats()) {
        filters << format.filter;
    }
    return filters;
}

QString SaveImageViewModel::preferredFilter() const
{
    return formatOf(m_settings->value(SessionProperties::Options::IMAGE_FORMAT).toString()).filter;
}

QString SaveImageViewModel::suggestedPath(const QString &filter) const
{
    const QFileInfo picture{m_document->path()};
    return picture.dir().filePath(picture.completeBaseName() + "-graded." + formatOf(filter).suffix);
}

std::expected<QString, QString> SaveImageViewModel::save(QString path, const QString &filter)
{
    if (!m_document->isOpen()) {
        return std::unexpected(tr("No picture is open."));
    }
    const ImageExportUtils::ImageFormat format = formatOf(filter);
    m_settings->setValue(SessionProperties::Options::IMAGE_FORMAT, format.filter);
    if (QFileInfo{path}.suffix().isEmpty()) {
        path += "." + format.suffix;
    }
    if (QFileInfo{path}.absoluteFilePath() == QFileInfo{m_document->path()}.absoluteFilePath()) {
        return std::unexpected(
            tr("The picture would replace the one it was made from: choose another name."));
    }

    ConcurrentUtils::run(
        this,
        [image = m_document->image(),
         settings = m_document->settings(),
         pipeline = m_pipeline,
         path,
         format] { return ImageExportUtils::saveImage(*image, settings, *pipeline, path, format); },
        [this, path](const std::expected<void, QString> &outcome) {
            if (outcome) {
                Q_EMIT saved(path);
            } else {
                Q_EMIT saveFailed(outcome.error());
            }
        });
    return path;
}
