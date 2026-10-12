// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAbstractScrollArea>
#include <QImage>

/**
 * Shows an image, centred, fitted to the window or zoomed, and dragged with the mouse. The wheel
 * zooms around the cursor. A file dropped on it is reported with fileDropped(), not opened: what
 * to do with it is the client's business.
 *
 * In pick mode, a click picks a point instead of dragging: the cursor is a cross with a circle,
 * the area sampled, and the click is reported with pointPicked(), in pixels of the full image.
 * Escape leaves pick mode. What the point is for is the client's business too.
 *
 * The image shown may be smaller than the image it stands for, e.g. a quick preview of a large
 * picture: the zoom is always relative to the full size, so replacing the preview with the full
 * image keeps the view where it is.
 */
class ImageView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    /// Shows the image, standing for an image of `fullSize`; of its own size when not given.
    void setImage(const QImage &image, QSize fullSize = {});
    void clear();

    QSize fullSize() const { return m_fullSize; }
    /// Screen pixels per pixel of the full image.
    double zoom() const { return m_zoom; }
    bool isFitToWindow() const { return m_fit; }

    /// The radius of the area a pick samples, in screen pixels, whatever the zoom.
    static constexpr double PICK_RADIUS = 5.0;

    bool isPickMode() const { return m_picking; }
    /// Enters or leaves pick mode, without emitting pickCancelled().
    void setPickMode(bool picking);

public Q_SLOTS:
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    /// One pixel of the full image per screen pixel, centred.
    void actualSize();

Q_SIGNALS:
    void zoomChanged(double zoom);
    void fileDropped(const QString &path);
    /**
     * In pick mode, the user clicked the image at `point`, in pixels of the full image; the area
     * sampled is the disc of `radius` around it, in the same pixels.
     */
    void pointPicked(QPointF point, double radius);
    /// The user left pick mode with Escape.
    void pickCancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool viewportEvent(QEvent *event) override;

private:
    double fitZoom() const;
    /// Zooms to `zoom`, keeping the point of the image under `anchor`, in viewport pixels.
    void setZoom(double zoom, QPointF anchor);
    void updateScrollBars();
    /// Where the top-left corner of the image is, in viewport pixels.
    QPointF imageOrigin() const;
    const QImage &scaledForScreen();

    QImage m_image;
    QSize m_fullSize;
    double m_zoom = 1.0;
    bool m_fit = true;

    QImage m_scaled;
    QSize m_scaledFor;

    bool m_picking = false;
    QPointF m_cursor;
    bool m_panning = false;
    QPoint m_panStart;
    QPoint m_scrollAtPanStart;
};
