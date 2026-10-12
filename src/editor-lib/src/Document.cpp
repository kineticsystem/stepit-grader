// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/Document.h"

#include "imageio/Error.h"
#include "imageio/ImageReader.h"
#include "imageio/SettingsStore.h"

#include <QtConcurrent>

#include <filesystem>

namespace {

std::filesystem::path toPath(const QString &path)
{
    return std::filesystem::path{path.toStdU16String()};
}

} // namespace

Document::Document(
    std::shared_ptr<const imageio::ImageReader> reader,
    std::shared_ptr<const imageio::SettingsStore> store,
    std::vector<core::Parameter> parameters,
    std::chrono::milliseconds saveDelay,
    QObject *parent)
    : QObject{parent}
    , m_reader{std::move(reader)}
    , m_store{std::move(store)}
    , m_parameters{std::move(parameters)}
{
    m_files.setMaxThreadCount(1);
    connect(&m_opening, &QFutureWatcher<Loaded>::finished, this, &Document::finishOpening);
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(saveDelay);
    connect(&m_saveTimer, &QTimer::timeout, this, &Document::save);
}

Document::~Document()
{
    // The jobs report to this document: none may still be running once it is gone.
    m_files.waitForDone();
}

void Document::open(const QString &path)
{
    if (m_opening.isRunning()) {
        return;
    }
    // The settings waiting to be saved belong to the picture open, not to the next one: written
    // first, as the files are read and written in turn.
    save();
    m_openingPath = path;
    m_opening.setFuture(
        QtConcurrent::run(
            &m_files,
            [reader = m_reader, store = m_store, parameters = m_parameters, file = toPath(path)] {
                Loaded loaded;
                try {
                    loaded.image = std::make_shared<const core::Image>(reader->read(file));
                    loaded.settings = store->load(file, parameters).value_or(core::Settings{});
                } catch (const std::exception &e) {
                    loaded.image.reset();
                    loaded.error = QString::fromUtf8(e.what());
                }
                return loaded;
            }));
}

void Document::finishOpening()
{
    Loaded loaded = m_opening.result();
    if (!loaded.image) {
        Q_EMIT openFailed(loaded.error);
        return;
    }
    // The sliders stay live while the next picture is read: changes made meanwhile are the old
    // picture's.
    save();
    m_path = m_openingPath;
    m_image = std::move(loaded.image);
    m_saved = loaded.settings;
    m_opened = loaded.settings;
    m_settings = loaded.settings;
    m_modified = false;
    m_lastSave.reset();
    Q_EMIT opened();
    Q_EMIT settingsChanged(m_settings);
    Q_EMIT modifiedChanged(false);
}

QString Document::settingsPath() const
{
    return m_path.isEmpty()
               ? QString{}
               : QString::fromStdU16String(m_store->location(toPath(m_path)).u16string());
}

bool Document::isModified() const
{
    return m_modified;
}

QStringList Document::nameFilters() const
{
    QStringList filters;
    for (const std::string &ext : m_reader->extensions()) {
        filters << "*." + QString::fromStdString(ext);
    }
    return filters;
}

bool Document::canOpen(const QString &path) const
{
    return m_reader->canRead(toPath(path));
}

void Document::setSettings(const core::Settings &settings)
{
    if (!settings.sameAs(m_settings, m_parameters)) {
        changeSettings(settings);
    }
}

void Document::resetSettings()
{
    setSettings(core::Settings{});
}

void Document::revertSettings()
{
    setSettings(m_opened);
}

void Document::save()
{
    m_saveTimer.stop();
    if (!isOpen() || !m_modified
        || (isSaving() && m_lastSave && m_lastSave->sameAs(m_settings, m_parameters))) {
        return;
    }
    ++m_saving;
    m_lastSave = m_settings;
    (void) QtConcurrent::run(
        &m_files,
        [this, store = m_store, parameters = m_parameters, path = m_path, settings = m_settings] {
            Written written{path, settings, {}};
            try {
                store->save(toPath(path), settings, parameters);
            } catch (const std::exception &e) {
                written.error = QString::fromUtf8(e.what());
            }
            QMetaObject::invokeMethod(
                this, [this, written] { finishSaving(written); }, Qt::QueuedConnection);
        });
}

void Document::finishSaving(const Written &written)
{
    --m_saving;
    const QString location = QString::fromStdU16String(
        m_store->location(toPath(written.path)).u16string());
    if (!written.error.isEmpty()) {
        Q_EMIT saveFailed(written.error);
        return;
    }
    // The settings of a picture since left were written: only the reports are for the window.
    if (written.path == m_path) {
        m_saved = written.settings;
        // Changes made while it was written wait for the next save.
        const bool modified = !m_settings.sameAs(m_saved, m_parameters);
        if (modified != m_modified) {
            m_modified = modified;
            Q_EMIT modifiedChanged(modified);
        }
    }
    Q_EMIT saved(location);
}

void Document::changeSettings(const core::Settings &settings)
{
    m_settings = settings;
    Q_EMIT settingsChanged(m_settings);
    const bool modified = !m_settings.sameAs(m_saved, m_parameters);
    if (modified != m_modified) {
        m_modified = modified;
        Q_EMIT modifiedChanged(modified);
    }
    // Each change starts the wait again, so a save follows the last of a quick series.
    if (modified) {
        m_saveTimer.start();
    } else {
        m_saveTimer.stop();
    }
}

QString Document::settingsNameFilter() const
{
    return "*." + QString::fromStdString(m_store->extension());
}

bool Document::loadSettings(const QString &file)
{
    if (!isOpen()) {
        return false;
    }
    (void) QtConcurrent::run(
        &m_files, [this, store = m_store, parameters = m_parameters, file, picture = m_path] {
            std::optional<core::Settings> settings;
            QString error;
            try {
                settings = store->loadFile(toPath(file), parameters);
            } catch (const std::exception &e) {
                error = QString::fromUtf8(e.what());
            }
            QMetaObject::invokeMethod(
                this,
                [this, file, picture, settings = std::move(settings), error] {
                    // Settings asked for a picture since left are not the new one's.
                    if (picture != m_path) {
                        return;
                    }
                    if (!settings) {
                        Q_EMIT settingsLoadFailed(error);
                        return;
                    }
                    setSettings(*settings);
                    Q_EMIT settingsLoaded(file);
                },
                Qt::QueuedConnection);
        });
    return true;
}
