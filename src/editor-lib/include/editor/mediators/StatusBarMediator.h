// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>

class CameraLookViewModel;
class Document;
class ImageView;
class LeavePictureViewModel;
class PreviewRenderer;
class QLabel;
class QStatusBar;
class SaveImageViewModel;
class WhiteBalanceViewModel;

/**
 * What the window says while it works, in its status bar: the picture open and its size, Before,
 * whether a preview or the full picture is shown, the zoom, and a message for each thing done,
 * saved or failed by the document and the view models; and the wait cursor while a picture opens.
 * It makes the bar's labels; each follow() connects one source, so the window gives it only what
 * it has.
 */
class StatusBarMediator : public QObject
{
    Q_OBJECT

public:
    explicit StatusBarMediator(QStatusBar *statusBar, QObject *parent = nullptr);

    /// The picture opened or not, its settings rendered, loaded and saved; the raw file's hint.
    void followDocument(const Document *document, const CameraLookViewModel *cameraLook);
    /// A preview shown, or the full picture.
    void followRenderer(const PreviewRenderer *renderer);
    void followView(const ImageView *view);
    void followSaveImage(const SaveImageViewModel *saveImage);
    void followWhiteBalance(const WhiteBalanceViewModel *whiteBalance);
    void followLeavePicture(const LeavePictureViewModel *leavePicture);

public Q_SLOTS:
    /// The picture being saved to `path`, in the background.
    void showSaving(const QString &path);
    /// Before, while the picture before its edit is shown.
    void showBefore(bool before);
    /// A picture is being rendered, e.g. for Before and After.
    void showRendering();

private:
    QStatusBar *m_statusBar;
    QLabel *m_fileLabel;
    QLabel *m_beforeLabel;
    QLabel *m_renderLabel;
    QLabel *m_zoomLabel;
};
