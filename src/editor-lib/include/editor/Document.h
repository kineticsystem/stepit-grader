// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "core/Parameter.h"
#include "core/Settings.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

namespace imageio {
class ImageReader;
class SettingsStore;
} // namespace imageio

/**
 * The picture being edited: its file, its pixels, and its settings: those it was opened with, those
 * its sidecar holds, and the current ones. It reads and saves through the reader and the store it is
 * given, so tests give it fakes, and it knows nothing of the window.
 *
 * It saves by itself: `saveDelay` after the last change of the settings, so a slider dragged writes
 * the sidecar once, when it stops. A save that fails is reported, and tried again at the next change.
 */
class Document : public QObject
{
    Q_OBJECT

public:
    Document(
        std::shared_ptr<const imageio::ImageReader> reader,
        std::shared_ptr<const imageio::SettingsStore> store,
        std::vector<core::Parameter> parameters,
        std::chrono::milliseconds saveDelay = std::chrono::seconds{1},
        QObject *parent = nullptr);

    /**
     * Saves the settings of the picture open, if they wait to be, then reads the image and its
     * settings in the background; ends with opened() or openFailed().
     */
    void open(const QString &path);

    bool isOpen() const { return m_image != nullptr; }
    bool isOpening() const { return m_opening.isRunning(); }
    QString path() const { return m_path; }
    QString settingsPath() const;
    std::shared_ptr<const core::Image> image() const { return m_image; }

    const core::Settings &settings() const { return m_settings; }
    /**
     * Whether the settings are not written yet: they differ from those the sidecar holds, or, with
     * no sidecar, from the defaults. True while a save waits, and after one failed.
     */
    bool isModified() const;

    /// The file names of settings it loads, for an open dialog, e.g. "*.xmp".
    QString settingsNameFilter() const;

    /// The file names it reads, for an open dialog, e.g. "*.tif *.dng".
    QStringList nameFilters() const;
    bool canOpen(const QString &path) const;

public Q_SLOTS:
    void setSettings(const core::Settings &settings);
    /// Every parameter back to its default.
    void resetSettings();
    /// The settings back to those the picture had when it was opened: its sidecar's, or none.
    void revertSettings();
    /**
     * Applies the settings of a file of the store's kind, e.g. a sidecar saved for another
     * picture, to this one: every value, those the file lacks at their defaults. A change like any
     * other, so saved and reverted like one. Ends with settingsLoaded() or settingsLoadFailed().
     */
    bool loadSettings(const QString &file);
    /**
     * Saves the settings now, if they are not written yet; ends with saved() or saveFailed().
     * Returns false only when they could not be written.
     */
    bool save();

Q_SIGNALS:
    void opened();
    void openFailed(const QString &message);
    void settingsChanged(const core::Settings &settings);
    void modifiedChanged(bool modified);
    void saved(const QString &settingsPath);
    void saveFailed(const QString &message);
    void settingsLoaded(const QString &file);
    void settingsLoadFailed(const QString &message);

private:
    struct Loaded
    {
        std::shared_ptr<const core::Image> image;
        core::Settings settings;
        QString error;
    };

    void finishOpening();
    void changeSettings(const core::Settings &settings);

    std::shared_ptr<const imageio::ImageReader> m_reader;
    std::shared_ptr<const imageio::SettingsStore> m_store;
    std::vector<core::Parameter> m_parameters;

    QFutureWatcher<Loaded> m_opening;
    QString m_openingPath;

    QString m_path;
    std::shared_ptr<const core::Image> m_image;
    core::Settings m_settings;
    /// What the sidecar holds, or the defaults without one: a save writes only what differs.
    core::Settings m_saved;
    /// The settings the picture was opened with, for revertSettings().
    core::Settings m_opened;
    bool m_modified = false;
    QTimer m_saveTimer;
};
