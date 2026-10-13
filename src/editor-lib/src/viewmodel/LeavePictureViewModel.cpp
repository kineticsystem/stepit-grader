// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/viewmodel/LeavePictureViewModel.h"
#include "editor/Document.h"

#include <QFileInfo>

#include <utility>

LeavePictureViewModel::LeavePictureViewModel(Document *document, QObject *parent)
    : QObject{parent}
    , m_document{document}
{
    connect(m_document, &Document::saved, this, [this] { continueWhenSaved(false); });
    connect(m_document, &Document::saveFailed, this, [this] { continueWhenSaved(true); });
}

std::expected<void, LeavePictureViewModel::Refusal> LeavePictureViewModel::open(const QString &path)
{
    if (!QFileInfo::exists(path)) {
        return std::unexpected(Refusal::Missing);
    }
    if (!m_document->canOpen(path)) {
        return std::unexpected(Refusal::NotAPicture);
    }
    if (m_document->isOpening()) {
        return std::unexpected(Refusal::Busy);
    }
    whenSaved([this, path] {
        Q_EMIT opening(path);
        m_document->open(path);
    });
    return {};
}

bool LeavePictureViewModel::close()
{
    if (m_closing || (!m_document->isModified() && !m_document->isSaving())) {
        return true;
    }
    whenSaved([this] {
        m_closing = true;
        Q_EMIT readyToClose();
    });
    return false;
}

void LeavePictureViewModel::discard()
{
    if (m_asking) {
        m_asking = false;
        std::exchange(m_waiting, {})();
    }
}

void LeavePictureViewModel::stay()
{
    if (m_asking) {
        m_asking = false;
        m_waiting = {};
    }
}

void LeavePictureViewModel::whenSaved(std::function<void()> then)
{
    // The last asked replaces one still waiting, e.g. a second picture chosen during a save.
    m_waiting = std::move(then);
    m_asking = false;
    m_document->save();
    continueWhenSaved(false);
}

void LeavePictureViewModel::continueWhenSaved(bool failed)
{
    // A save asked before may end first: what waits, waits for the last one.
    if (!m_waiting || m_asking || m_document->isSaving()) {
        return;
    }
    if (m_document->isModified()) {
        if (!failed) {
            // Changed again while it was written: written again.
            m_document->save();
            if (m_document->isSaving()) {
                return;
            }
        }
        // Only here would the edit be lost, so only here is the user asked.
        m_asking = true;
        Q_EMIT editWouldBeLost(QFileInfo{m_document->path()}.fileName());
        return;
    }
    std::exchange(m_waiting, {})();
}
