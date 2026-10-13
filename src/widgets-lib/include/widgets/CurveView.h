// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QPointF>
#include <QWidget>

#include <vector>

/**
 * A tone curve, drawn: the tones before the edit along the bottom, from black on the left to white
 * on the right, and after it up the side. The diagonal, where nothing changes, is drawn faintly
 * beneath it, and the borders between the regions the sliders move as dotted lines. It only shows
 * the curve it is given: the sliders change it.
 */
class CurveView : public QWidget
{
    Q_OBJECT

public:
    explicit CurveView(QWidget *parent = nullptr);

    /**
     * The curve, as points from (0, 0) to (1, 1) in order of x, the tone before and after; and the
     * borders between its regions, from 0 to 1.
     */
    void setCurve(std::vector<QPointF> points, std::vector<double> borders);

    const std::vector<QPointF> &points() const { return m_points; }
    const std::vector<double> &borders() const { return m_borders; }

    /// Where the tone (x, y) is drawn, in the widget's pixels.
    QPointF toWidget(QPointF tone) const;

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRectF plotArea() const;

    std::vector<QPointF> m_points;
    std::vector<double> m_borders;
};
