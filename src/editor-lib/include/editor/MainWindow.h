// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/widgets/AdjustmentsPanel.h"

#include <QMainWindow>

class Document;
class ImageView;
class PreviewRenderer;
class QLabel;

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
    /// Opens the file, after asking to save the settings of the picture open, if changed.
    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createActions();
    void chooseFile();
    /// Whether the user saved the changes or let them go; false when cancelled.
    bool maybeSave();
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
};
