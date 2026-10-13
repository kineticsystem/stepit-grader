// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/mediators/StatusBarMediator.h"
#include "editor/Document.h"
#include "editor/PreviewRenderer.h"
#include "editor/viewmodel/CameraLookViewModel.h"
#include "editor/viewmodel/LeavePictureViewModel.h"
#include "editor/viewmodel/SaveImageViewModel.h"
#include "editor/viewmodel/WhiteBalanceViewModel.h"
#include "widgets/ImageView.h"

#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QStatusBar>

namespace {

/// How long a message shows that something was saved or loaded, which happens after every edit.
constexpr int SAVED_MESSAGE_MS = 3000;
constexpr int LOADED_MESSAGE_MS = 5000;
/// How long it shows a failed save; the title's * stays until a save succeeds.
constexpr int SAVE_FAILED_MESSAGE_MS = 15000;
/// How long it shows what a pick of the white balance did.
constexpr int PICK_MESSAGE_MS = 6000;
/// How long it tells that a raw file can be matched to its camera's look.
constexpr int RAW_HINT_MS = 10000;

} // namespace

StatusBarMediator::StatusBarMediator(QStatusBar *statusBar, QObject *parent)
    : QObject{parent}
    , m_statusBar{statusBar}
    , m_fileLabel{new QLabel{statusBar}}
    , m_beforeLabel{new QLabel{tr("Before"), statusBar}}
    , m_renderLabel{new QLabel{statusBar}}
    , m_zoomLabel{new QLabel{statusBar}}
{
    m_fileLabel->setObjectName("file");
    m_beforeLabel->setObjectName("before");
    m_renderLabel->setObjectName("render");
    m_zoomLabel->setObjectName("zoom");
    m_statusBar->addWidget(m_fileLabel, 1);
    m_statusBar->addPermanentWidget(m_beforeLabel);
    m_statusBar->addPermanentWidget(m_renderLabel);
    m_statusBar->addPermanentWidget(m_zoomLabel);
    QFont bold = m_beforeLabel->font();
    bold.setBold(true);
    m_beforeLabel->setFont(bold);
    m_beforeLabel->hide();
}

void StatusBarMediator::followDocument(
    const Document *document, const CameraLookViewModel *cameraLook)
{
    connect(document, &Document::opened, this, [this, document, cameraLook] {
        QApplication::restoreOverrideCursor();
        const auto image = document->image();
        m_fileLabel->setText(tr("%1 — %2 × %3")
                                 .arg(QDir::toNativeSeparators(document->path()))
                                 .arg(image->width)
                                 .arg(image->height));
        if (cameraLook->matchesOpenPicture()) {
            m_statusBar->showMessage(
                tr("A raw file: Edit > Match Camera Look grades it like its camera's JPEG"),
                RAW_HINT_MS);
        }
    });
    // Before its warning: a dialog under the wait cursor would look busy.
    connect(document, &Document::openFailed, this, [] { QApplication::restoreOverrideCursor(); });
    connect(document, &Document::settingsChanged, this, &StatusBarMediator::showRendering);
    connect(document, &Document::settingsLoaded, this, [this](const QString &file) {
        m_statusBar->showMessage(
            tr("Loaded the settings of %1").arg(QDir::toNativeSeparators(file)), LOADED_MESSAGE_MS);
    });
    connect(document, &Document::saved, this, [this](const QString &path) {
        m_statusBar
            ->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(path)), SAVED_MESSAGE_MS);
    });
    // The document saves by itself and tries again at the next change: a dialog for each failure
    // would interrupt every slider move.
    connect(document, &Document::saveFailed, this, [this](const QString &message) {
        m_statusBar
            ->showMessage(tr("Cannot save the settings: %1").arg(message), SAVE_FAILED_MESSAGE_MS);
    });
}

void StatusBarMediator::followRenderer(const PreviewRenderer *renderer)
{
    connect(renderer, &PreviewRenderer::rendered, this, [this](const QImage &, QSize, bool full) {
        m_renderLabel->setText(full ? QString{} : tr("Preview"));
    });
}

void StatusBarMediator::followView(const ImageView *view)
{
    connect(view, &ImageView::zoomChanged, this, [this](double zoom) {
        m_zoomLabel->setText(QString{"%1%"}.arg(qRound(zoom * 100)));
    });
}

void StatusBarMediator::followSaveImage(const SaveImageViewModel *saveImage)
{
    connect(saveImage, &SaveImageViewModel::saved, this, [this](const QString &path) {
        m_statusBar
            ->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(path)), SAVED_MESSAGE_MS);
    });
    // The action says why in a window.
    connect(saveImage, &SaveImageViewModel::saveFailed, m_statusBar, &QStatusBar::clearMessage);
}

void StatusBarMediator::followWhiteBalance(const WhiteBalanceViewModel *whiteBalance)
{
    const auto show = [this](const QString &message) {
        m_statusBar->showMessage(message, PICK_MESSAGE_MS);
    };
    connect(whiteBalance, &WhiteBalanceViewModel::picked, this, show);
    connect(whiteBalance, &WhiteBalanceViewModel::failed, this, show);
}

void StatusBarMediator::followLeavePicture(const LeavePictureViewModel *leavePicture)
{
    connect(leavePicture, &LeavePictureViewModel::opening, this, [this](const QString &path) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        m_fileLabel->setText(tr("Opening %1…").arg(QDir::toNativeSeparators(path)));
    });
}

void StatusBarMediator::showSaving(const QString &path)
{
    m_statusBar->showMessage(tr("Saving %1…").arg(QDir::toNativeSeparators(path)));
}

void StatusBarMediator::showBefore(bool before)
{
    m_beforeLabel->setVisible(before);
}

void StatusBarMediator::showRendering()
{
    m_renderLabel->setText(tr("Rendering…"));
}
