// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/AdjustmentsPanel.h"

#include <QMainWindow>
#include <QString>

#include <utility>
#include <vector>

class CameraLookViewModel;
class Document;
class LeavePictureViewModel;
class SaveImageViewModel;
class ImageView;
class PreviewRenderer;
class StatusBarMediator;
class RecentFilesMenu;
class WhiteBalanceViewModel;

/**
 * The editor's window: the picture in the middle, the tools on the right, and the menus. It
 * connects the parts it is given, which know nothing of each other: the document, the renderer,
 * the image view and the tools panel.
 *
 * Its actions do no work of their own on the user interface's thread: the files are read and
 * written by the document, the pictures rendered by the renderer, and the white balance measured
 * in the background.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(
        Document *document,
        PreviewRenderer *renderer,
        SaveImageViewModel *saveImage,
        CameraLookViewModel *cameraLook,
        LeavePictureViewModel *leavePicture,
        WhiteBalanceViewModel *whiteBalance,
        std::vector<AdjustmentsPanel::Section> sections,
        QWidget *parent = nullptr);

public Q_SLOTS:
    /// Opens the file, after saving the settings of the picture open that wait to be; says why not.
    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createActions();
    /// The menu of the application's own settings: the theme.
    void createSettingsMenu();
    /// Gives the action the icon `name` of the theme, and again when the theme changes.
    void setThemedIcon(QAction *action, const QString &name);
    void refreshIcons();
    void onOpened();
    void updateTitle();

    Document *m_document;
    PreviewRenderer *m_renderer;
    SaveImageViewModel *m_saveImage;
    CameraLookViewModel *m_cameraLook;
    LeavePictureViewModel *m_leavePicture;
    WhiteBalanceViewModel *m_whiteBalance;
    ImageView *m_view;
    AdjustmentsPanel *m_panel;
    QAction *m_pickWhite = nullptr;
    /// The actions with an icon, and the icon's name, drawn again for a new theme.
    std::vector<std::pair<QAction *, QString>> m_themedIcons;
    RecentFilesMenu *m_recentFiles = nullptr;
    StatusBarMediator *m_status;
};
