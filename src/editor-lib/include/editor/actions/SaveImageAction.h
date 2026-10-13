// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAction>

class ImageExporter;

/**
 * File > Save Image…: asks for a file and its format, and has the exporter save the picture there;
 * tells the user when the exporter refuses the file or the save fails. The dialogs only: what is
 * saved, and where it may be, is the exporter's.
 */
class SaveImageAction : public QAction
{
    Q_OBJECT

public:
    SaveImageAction(ImageExporter *exporter, QWidget *parent);

Q_SIGNALS:
    /// The exporter took the file, and saves the picture to it in the background.
    void started(const QString &path);

private:
    void execute();

    ImageExporter *m_exporter;
    QWidget *m_parent;
};
