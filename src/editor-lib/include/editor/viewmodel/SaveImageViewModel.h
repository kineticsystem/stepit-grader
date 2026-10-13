// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Pipeline.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <expected>
#include <memory>

class Document;
class QSettings;

/**
 * Saves the picture open with its grade, at its full size, in the background: which file it
 * suggests, which format it offers first, and which files it refuses. It asks nothing of the user,
 * so the action that does, `SaveImageAction`, holds only the dialogs, and tests reach the rules.
 *
 * The format used last is kept in the `QSettings` it is given, so the next save offers it first.
 */
class SaveImageViewModel : public QObject
{
    Q_OBJECT

public:
    SaveImageViewModel(
        Document *document,
        std::shared_ptr<const core::Pipeline> pipeline,
        QSettings *settings,
        QObject *parent = nullptr);

    /// The filters of `imageFormats()`, in their order, for a save dialog.
    QStringList filters() const;
    /// The filter of the format used last, or of the first when none was, or it is gone.
    QString preferredFilter() const;
    /// Beside the picture, its name followed by "-graded", with the suffix of `filter`'s format.
    QString suggestedPath(const QString &filter) const;

    /**
     * Starts saving the picture as the format of `filter`, the first for a filter it does not
     * know, to `path`, given that format's suffix when it has none; ends with saved() or
     * saveFailed(). Returns the path written, or, at once and without saving, why it refuses:
     * no picture is open, or the file is the picture itself.
     */
    std::expected<QString, QString> save(QString path, const QString &filter);

Q_SIGNALS:
    void saved(const QString &path);
    void saveFailed(const QString &message);

private:
    Document *m_document;
    std::shared_ptr<const core::Pipeline> m_pipeline;
    QSettings *m_settings;
};
