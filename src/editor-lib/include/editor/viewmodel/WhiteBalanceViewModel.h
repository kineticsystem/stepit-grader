// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QPointF>
#include <QString>

class Document;

/**
 * The white balance's eyedropper without its view: measures the area picked in the background,
 * and sets the temperature and the tint that make it neutral, the other sliders as they are by
 * then. A pick measured on a picture since left is dropped. It tells the views with picked() or
 * failed(), each with a message for the user.
 */
class WhiteBalanceViewModel : public QObject
{
    Q_OBJECT

public:
    explicit WhiteBalanceViewModel(Document *document, QObject *parent = nullptr);

    /// Measures the area around `point`, of `radius`, both in pixels of the picture.
    void pick(QPointF point, double radius);

Q_SIGNALS:
    void picked(const QString &message);
    void failed(const QString &message);

private:
    Document *m_document;
};
