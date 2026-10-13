// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/ImageExporter.h"
#include "editor/Document.h"
#include "editor/ImageExport.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QSettings>
#include <QtConcurrent>

#include <algorithm>

namespace {

/// The format the picture was last saved as, by its filter: the next save offers it first.
constexpr const char *IMAGE_FORMAT_KEY = "imageFormat";

/// The format of `filter`, or the first for a filter that names none.
const ImageFormat &formatOf(const QString &filter)
{
    const auto &formats = imageFormats();
    const auto found = std::ranges::find(formats, filter, &ImageFormat::filter);
    return found != formats.end() ? *found : formats.front();
}

} // namespace

ImageExporter::ImageExporter(
    Document *document,
    std::shared_ptr<const core::Pipeline> pipeline,
    QSettings *settings,
    QObject *parent)
    : QObject{parent}
    , m_document{document}
    , m_pipeline{std::move(pipeline)}
    , m_settings{settings}
{}

QStringList ImageExporter::filters() const
{
    QStringList filters;
    for (const ImageFormat &format : imageFormats()) {
        filters << format.filter;
    }
    return filters;
}

QString ImageExporter::preferredFilter() const
{
    return formatOf(m_settings->value(IMAGE_FORMAT_KEY).toString()).filter;
}

QString ImageExporter::suggestedPath(const QString &filter) const
{
    const QFileInfo picture{m_document->path()};
    return picture.dir().filePath(picture.completeBaseName() + "-graded." + formatOf(filter).suffix);
}

std::expected<QString, QString> ImageExporter::save(QString path, const QString &filter)
{
    if (!m_document->isOpen()) {
        return std::unexpected(tr("No picture is open."));
    }
    const ImageFormat format = formatOf(filter);
    m_settings->setValue(IMAGE_FORMAT_KEY, format.filter);
    if (QFileInfo{path}.suffix().isEmpty()) {
        path += "." + format.suffix;
    }
    if (QFileInfo{path}.absoluteFilePath() == QFileInfo{m_document->path()}.absoluteFilePath()) {
        return std::unexpected(
            tr("The picture would replace the one it was made from: choose another name."));
    }

    using Outcome = std::expected<void, QString>;
    auto *watcher = new QFutureWatcher<Outcome>{this};
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, path] {
        watcher->deleteLater();
        const Outcome outcome = watcher->result();
        if (outcome) {
            Q_EMIT saved(path);
        } else {
            Q_EMIT saveFailed(outcome.error());
        }
    });
    watcher->setFuture(
        QtConcurrent::run(
            [image = m_document->image(),
             settings = m_document->settings(),
             pipeline = m_pipeline,
             path,
             format] { return saveImage(*image, settings, *pipeline, path, format); }));
    return path;
}
