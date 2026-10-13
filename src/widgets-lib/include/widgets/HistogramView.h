// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QImage>
#include <QWidget>

#include <array>

/// How many pixels of an image sit at each level of red, green, blue and luminance, 0 to 255:
/// `HistogramUtils::histogramOf()` counts them.
struct Histogram
{
    static constexpr int LEVELS = 256;
    enum Channel { RED, GREEN, BLUE, LUMINANCE, CHANNELS };

    std::array<std::array<int, LEVELS>, CHANNELS> counts{};
    /// The pixels counted.
    int pixels = 0;
    /// The pixels with a channel at 0, or at 255: the shadows and the highlights clipped.
    int clippedBlack = 0;
    int clippedWhite = 0;
};

/**
 * A histogram, drawn: red, green and blue overlaid, the luminance as a line over them, from black
 * on the left to white on the right. A triangle in a top corner lights up when pixels are clipped
 * at that end, with their share of the picture below it. It only shows the histogram it is given.
 */
class HistogramView : public QWidget
{
    Q_OBJECT

public:
    explicit HistogramView(QWidget *parent = nullptr);

    void setHistogram(const Histogram &histogram);
    const Histogram &histogram() const { return m_histogram; }

    /// The share of the pixels clipped at an end, 0 to 1, below which its triangle stays dark.
    static constexpr double CLIPPING_SHOWN = 0.0001;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Histogram m_histogram;
};
