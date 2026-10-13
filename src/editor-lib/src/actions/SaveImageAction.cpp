// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/actions/SaveImageAction.h"
#include "editor/viewmodel/SaveImageViewModel.h"

#include <QFileDialog>
#include <QMessageBox>

SaveImageAction::SaveImageAction(SaveImageViewModel *viewModel, QWidget *parent)
    : QAction{tr("Save &Image…"), parent}
    , m_viewModel{viewModel}
    , m_parent{parent}
{
    setShortcut(QKeySequence{tr("Ctrl+Shift+E")});
    setToolTip(tr("Save the picture with its grade, at its full size"));
    connect(this, &QAction::triggered, this, &SaveImageAction::execute);
    connect(m_viewModel, &SaveImageViewModel::saveFailed, this, [this](const QString &message) {
        QMessageBox::warning(m_parent, tr("Cannot Save the Image"), message);
    });
}

void SaveImageAction::execute()
{
    QString filter = m_viewModel->preferredFilter();
    const QString path = QFileDialog::getSaveFileName(
        m_parent,
        tr("Save Image"),
        m_viewModel->suggestedPath(filter),
        m_viewModel->filters().join(";;"),
        &filter);
    if (path.isEmpty()) {
        return;
    }
    const auto saving = m_viewModel->save(path, filter);
    if (!saving) {
        QMessageBox::warning(m_parent, tr("Save Image"), saving.error());
        return;
    }
    Q_EMIT started(*saving);
}
