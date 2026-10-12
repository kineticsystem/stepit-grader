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
    QObject *parent)
    : QObject{parent}
    , m_reader{std::move(reader)}
    , m_store{std::move(store)}
    , m_parameters{std::move(parameters)}
{
    connect(&m_opening, &QFutureWatcher<Loaded>::finished, this, &Document::finishOpening);
}

void Document::open(const QString &path)
{
    if (m_opening.isRunning()) {
        return;
    }
    m_openingPath = path;
    m_opening.setFuture(QtConcurrent::run(
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
    m_path = m_openingPath;
    m_image = std::move(loaded.image);
    m_saved = loaded.settings;
    m_settings = loaded.settings;
    m_modified = false;
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
    setSettings(m_saved);
}

bool Document::save()
{
    if (!isOpen()) {
        return false;
    }
    try {
        m_store->save(toPath(m_path), m_settings, m_parameters);
    } catch (const std::exception &e) {
        Q_EMIT saveFailed(QString::fromUtf8(e.what()));
        return false;
    }
    m_saved = m_settings;
    m_modified = false;
    Q_EMIT modifiedChanged(false);
    Q_EMIT saved(settingsPath());
    return true;
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
}
