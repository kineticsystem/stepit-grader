// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/widgets/AdjustmentsPanel.h"

#include <QMainWindow>

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
    void chooseFile();
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
    RecentFilesMenu *m_recentFiles = nullptr;
};
