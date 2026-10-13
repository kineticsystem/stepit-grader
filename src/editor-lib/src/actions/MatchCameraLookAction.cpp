// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/actions/MatchCameraLookAction.h"
#include "editor/FileUtils.h"
#include "editor/WaitingDialog.h"
#include "editor/viewmodel/CameraLookViewModel.h"

#include <QMessageBox>

MatchCameraLookAction::MatchCameraLookAction(CameraLookViewModel *viewModel, QWidget *parent)
    : QAction{tr("&Match Camera Look…"), parent}
    , m_viewModel{viewModel}
    , m_parent{parent}
{
    setToolTip(tr("Find the grade that makes raw files look like their camera's JPEGs"));
    connect(this, &QAction::triggered, this, &MatchCameraLookAction::execute);
    connect(m_viewModel, &CameraLookViewModel::finished, this, &MatchCameraLookAction::showFound);
    connect(m_viewModel, &CameraLookViewModel::failed, this, &MatchCameraLookAction::showFailed);
}

void MatchCameraLookAction::execute()
{
    QStringList raws{m_viewModel->picture()};
    if (!m_viewModel->matchesOpenPicture()) {
        // In the picture's folder each time: the raw files it was made from are usually there.
        raws = FileUtils::getOpenFiles(
            m_parent,
            tr("Choose the Raw Files This Picture Was Made From"),
            m_viewModel->rawNameFilter(),
            nullptr,
            m_viewModel->folder());
    }
    const auto status = m_viewModel->start(raws);
    if (!status) {
        return;
    }
    m_waiting = new WaitingDialog{status, tr("Stop"), m_parent};
    m_waiting->setWindowTitle(tr("Matching the Camera's Look"));
    m_waiting->open();
}

void MatchCameraLookAction::closeWaiting()
{
    if (m_waiting) {
        m_waiting->accept();
        m_waiting->deleteLater();
    }
}

void MatchCameraLookAction::showFound()
{
    closeWaiting();
    const QString text = m_viewModel->summary() + "\n\n"
                         + tr(
                             "Apply it to the picture? It sets the white balance, the tone, the "
                             "tone curve and the colour mixer; the other sliders stay as they "
                             "are. The picture's settings keep it, as any grade.");
    if (QMessageBox::question(
            m_parent,
            tr("Match Camera Look"),
            text,
            QMessageBox::Apply | QMessageBox::Discard,
            QMessageBox::Apply)
        == QMessageBox::Apply) {
        m_viewModel->apply();
    }
}

void MatchCameraLookAction::showFailed(const QString &message)
{
    closeWaiting();
    QMessageBox::warning(m_parent, tr("Match Camera Look"), message);
}
