// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "core/Parameter.h"
#include "core/Settings.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>

#include <memory>
#include <optional>
#include <vector>

namespace imageio {
class ImageReader;
class SettingsStore;
} // namespace imageio

/**
 * The picture being edited: its file, its pixels, and its settings, the saved ones and the
 * current ones. It reads and saves through the reader and the store it is given, so tests give it
 * fakes, and it knows nothing of the window.
 */
class Document : public QObject
{
    Q_OBJECT

public:
    Document(
        std::shared_ptr<const imageio::ImageReader> reader,
        std::shared_ptr<const imageio::SettingsStore> store,
        std::vector<core::Parameter> parameters,
        QObject *parent = nullptr);

    /// Reads the image and its settings in the background; ends with opened() or openFailed().
    void open(const QString &path);

    bool isOpen() const { return m_image != nullptr; }
    bool isOpening() const { return m_opening.isRunning(); }
    QString path() const { return m_path; }
    QString settingsPath() const;
    std::shared_ptr<const core::Image> image() const { return m_image; }

    const core::Settings &settings() const { return m_settings; }
    /// Whether the settings differ from those saved, or, with none saved, from the defaults.
    bool isModified() const;

    /// The file names it reads, for an open dialog, e.g. "*.tif *.dng".
    QStringList nameFilters() const;
    bool canOpen(const QString &path) const;

public Q_SLOTS:
    void setSettings(const core::Settings &settings);
    /// Every parameter back to its default.
    void resetSettings();
    /// The settings back to those saved.
    void revertSettings();
    /// Saves the settings; ends with saved() or saveFailed(). Returns whether they were saved.
    bool save();

Q_SIGNALS:
    void opened();
    void openFailed(const QString &message);
    void settingsChanged(const core::Settings &settings);
    void modifiedChanged(bool modified);
    void saved(const QString &settingsPath);
    void saveFailed(const QString &message);

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
    core::Settings m_saved;
    bool m_modified = false;
};
