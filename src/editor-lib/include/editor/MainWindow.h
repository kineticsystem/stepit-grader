// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/AdjustmentsPanel.h"

#include <QMainWindow>

#include <functional>
#include <utility>
#include <vector>

struct WhiteBalancePick;
class Document;
class ImageView;
class PreviewRenderer;
class QLabel;
class RecentFilesMenu;

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
     * Runs `then` once the settings that wait to be saved are written, in the background; when
     * they cannot be, asks whether to go on without them, and forgets `then` if the user stays.
     */
    void whenSaved(std::function<void()> then);
    /// Goes on with the action waiting for a save, if the save asked has ended.
    void continueWhenSaved(bool failed);
    void onOpened();
    void updateTitle();
    void updateZoom(double zoom);
    /// Measures the white balance in the background, and sets it once measured.
    void pickWhiteBalanceAt(QPointF point, double radius);
    void showWhiteBalancePicked(const WhiteBalancePick &pick);

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
    /// The action waiting for the settings to be written, e.g. opening another picture.
    std::function<void()> m_whenSaved;
    /// The window closes: its settings are written, or the user chose to leave them.
    bool m_closing = false;
};
