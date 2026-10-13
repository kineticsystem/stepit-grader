// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>

#include <expected>
#include <functional>

class Document;

/**
 * Opening another picture and closing the window leave the picture open: which file may be
 * opened, and the wait for its settings to be written before it is left. A change made while they
 * are written is written in turn; only when a save failed, where the edit would be lost, does it
 * ask, with editWouldBeLost(), and wait for discard() or stay().
 *
 * It asks nothing of the user itself and calls no view: the window asks, and follows its signals.
 */
class LeavePictureViewModel : public QObject
{
    Q_OBJECT

public:
    /// Why a file is not opened.
    enum class Refusal {
        /// It no longer exists, e.g. a recent picture since moved or deleted.
        Missing,
        /// Not a kind of file the document reads.
        NotAPicture,
        /// Another picture is opening: one at a time.
        Busy,
    };

    explicit LeavePictureViewModel(Document *document, QObject *parent = nullptr);

    /**
     * Opens `path` once the settings of the picture open are written: emits opening() when it
     * asks the document to open it. Returns, at once, why it refuses the file.
     */
    std::expected<void, Refusal> open(const QString &path);
    /**
     * Whether the window may close now: nothing waits to be written, or it was written since a
     * close was asked. Otherwise it starts writing, and emits readyToClose() once done.
     */
    bool close();

    /// Leaves the picture with its edit unsaved, after editWouldBeLost().
    void discard();
    /// Stays on the picture, after editWouldBeLost(): what was waiting is forgotten.
    void stay();

Q_SIGNALS:
    void opening(const QString &path);
    /// The settings are written: close() now says yes.
    void readyToClose();
    /// The settings of the picture `fileName` cannot be written: leaving it would lose the edit.
    void editWouldBeLost(const QString &fileName);

private:
    /// Runs `then` once the settings are written, or after discard().
    void whenSaved(std::function<void()> then);
    /// Goes on with what waits, if the last save asked has ended.
    void continueWhenSaved(bool failed);

    Document *m_document;
    /// What waits for the settings to be written, e.g. opening another picture.
    std::function<void()> m_waiting;
    /// Waiting for discard() or stay().
    bool m_asking = false;
    /// The settings were written, or left, for a close: the next close() says yes.
    bool m_closing = false;
};
