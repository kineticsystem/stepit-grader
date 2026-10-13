// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/CameraMatch.h"
#include "core/Pipeline.h"
#include "editor/Status.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>
#include <optional>

class Document;

namespace imageio {
class CameraShotReader;
} // namespace imageio

/**
 * Edit > Match Camera Look… without its dialogs: which raw files are matched, the search in the
 * background, what the user is told of the grade found, and how it meets the picture's own. The
 * action that asks and shows, `MatchCameraLookAction`, follows it through its signals, and tests
 * drive it as the action does, with a fake reader.
 */
class CameraLookViewModel : public QObject
{
    Q_OBJECT

public:
    CameraLookViewModel(
        Document *document,
        std::shared_ptr<const core::Pipeline> pipeline,
        std::shared_ptr<const imageio::CameraShotReader> reader,
        QObject *parent = nullptr);

    /**
     * Whether the picture open is matched itself: a raw file holds its camera's JPEG. A TIFF does
     * not: the user chooses the raw files it was made from.
     */
    bool matchesOpenPicture() const;
    /// The path of the picture open.
    QString picture() const;
    /// The raw files the reader reads, for an open dialog, e.g. "Camera raw files (*.cr2 *.CR2)".
    QString rawNameFilter() const;
    /// Where to look for the raw files: the picture's folder.
    QString folder() const;

    bool isRunning() const { return m_status != nullptr; }
    /**
     * Starts matching `raws` in the background, and returns what follows it, to show and to stop
     * it; ends with finished() or failed(). Returns nothing, and starts nothing, while a match
     * runs or with no file.
     */
    std::shared_ptr<StatusController> start(const QStringList &raws);

    /// What the last match found, for the user to apply or not; empty before one.
    QString summary() const;
    /// Applies the grade the last match found: the sliders it sets, the others as they are.
    void apply();

Q_SIGNALS:
    void finished();
    void failed(const QString &message);

private:
    Document *m_document;
    std::shared_ptr<const core::Pipeline> m_pipeline;
    std::shared_ptr<const imageio::CameraShotReader> m_reader;
    /// The match running, or none.
    std::shared_ptr<StatusController> m_status;
    std::optional<core::match::Result> m_result;
};
