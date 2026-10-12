// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/widgets/AdjustmentsPanel.h"

#include <QMainWindow>

#include <utility>
#include <vector>

class Document;
class ImageView;
class PreviewRenderer;
class QLabel;
class RecentFilesMenu;

/**
 * The editor's window: the picture in the middle, the tools on the right, and the menus. It
 * connects the parts it is given, which know nothing of each other: the document, the renderer,
 * the image view and the tools panel.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(
        Document *document,
        PreviewRenderer *renderer,
        std::vector<AdjustmentsPanel::Section> sections,
        QWidget *parent = nullptr);

public Q_SLOTS:
    /// Opens the file, after saving the settings of the picture open that wait to be.
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
    /// Shows the picture before its edit, or after.
    void showBefore(bool before);
    void chooseFile();
    /// Asks for a file of settings, and applies it to the picture open.
    void chooseSettings();
    /**
     * Saves the settings that wait to be; when that fails, asks whether to go on without them.
     * Returns false when the user stays.
     */
    bool settingsSaved();
    void onOpened();
    void updateTitle();
    void updateZoom(double zoom);
    void pickWhiteBalanceAt(QPointF point, double radius);

    Document *m_document;
    PreviewRenderer *m_renderer;
    ImageView *m_view;
    AdjustmentsPanel *m_panel;
    QLabel *m_fileLabel;
    QLabel *m_zoomLabel;
    QLabel *m_renderLabel;
    QAction *m_pickWhite = nullptr;
    QAction *m_showBefore = nullptr;
    /// The actions with an icon, and the icon's name, drawn again for a new theme.
    std::vector<std::pair<QAction *, QString>> m_themedIcons;
    QLabel *m_beforeLabel;
    RecentFilesMenu *m_recentFiles = nullptr;
};
