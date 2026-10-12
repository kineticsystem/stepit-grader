// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/ImageView.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

/// The zoom step of zoomIn(), and of one notch of the wheel.
constexpr double ZOOM_STEP = 1.25;
/// The most zoomed in: 8 screen pixels per image pixel, to check a print's detail.
constexpr double MAX_ZOOM = 8.0;
/// The least zoomed out, besides fitting a large image.
constexpr double MIN_ZOOM = 0.02;

} // namespace

ImageView::ImageView(QWidget *parent)
    : QAbstractScrollArea{parent}
{
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    viewport()->setCursor(Qt::OpenHandCursor);
    viewport()->setBackgroundRole(QPalette::Dark);
    viewport()->setAutoFillBackground(true);
    setFrameShape(QFrame::NoFrame);
    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);
}

void ImageView::setImage(const QImage &image, QSize fullSize)
{
    const QSize size = fullSize.isValid() ? fullSize : image.size();
    const bool newImage = size != m_fullSize;
    m_image = image;
    m_fullSize = size;
    m_scaled = {};
    if (newImage) {
        m_fit = true;
    }
    if (m_fit) {
        m_zoom = fitZoom();
        Q_EMIT zoomChanged(m_zoom);
    }
    updateScrollBars();
    viewport()->update();
}

void ImageView::clear()
{
    m_image = {};
    m_fullSize = {};
    m_scaled = {};
    updateScrollBars();
    viewport()->update();
}

void ImageView::setPickMode(bool picking)
{
    m_picking = picking;
    m_panning = false;
    viewport()->setMouseTracking(picking);
    viewport()->setCursor(picking ? Qt::CrossCursor : Qt::OpenHandCursor);
    if (picking) {
        // So that Escape reaches it, whatever button turned pick mode on.
        setFocus();
    }
    viewport()->update();
}

void ImageView::zoomIn()
{
    setZoom(m_zoom * ZOOM_STEP, viewport()->rect().center());
}

void ImageView::zoomOut()
{
    setZoom(m_zoom / ZOOM_STEP, viewport()->rect().center());
}

void ImageView::fitToWindow()
{
    m_fit = true;
    m_zoom = fitZoom();
    updateScrollBars();
    viewport()->update();
    Q_EMIT zoomChanged(m_zoom);
}

void ImageView::actualSize()
{
    setZoom(1.0, viewport()->rect().center());
}

double ImageView::fitZoom() const
{
    if (m_fullSize.isEmpty()) {
        return 1.0;
    }
    const QSize area = maximumViewportSize();
    return std::min(
        {1.0,
         static_cast<double>(area.width()) / m_fullSize.width(),
         static_cast<double>(area.height()) / m_fullSize.height()});
}

void ImageView::setZoom(double zoom, QPointF anchor)
{
    if (m_fullSize.isEmpty()) {
        return;
    }
    const double clamped = std::clamp(zoom, std::min(MIN_ZOOM, fitZoom()), MAX_ZOOM);
    // The point of the full image under the anchor stays under it.
    const QPointF imagePoint = (anchor - imageOrigin()) / m_zoom;
    m_fit = false;
    m_zoom = clamped;
    m_scaled = {};
    updateScrollBars();
    const QPointF moved = imageOrigin() + imagePoint * m_zoom - anchor;
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + qRound(moved.x()));
    verticalScrollBar()->setValue(verticalScrollBar()->value() + qRound(moved.y()));
    viewport()->update();
    Q_EMIT zoomChanged(m_zoom);
}

void ImageView::updateScrollBars()
{
    const QSize content = (QSizeF(m_fullSize) * m_zoom).toSize();
    const QSize area = viewport()->size();
    horizontalScrollBar()->setRange(0, std::max(0, content.width() - area.width()));
    verticalScrollBar()->setRange(0, std::max(0, content.height() - area.height()));
    horizontalScrollBar()->setPageStep(area.width());
    verticalScrollBar()->setPageStep(area.height());
}

QPointF ImageView::imageOrigin() const
{
    const QSizeF content = QSizeF(m_fullSize) * m_zoom;
    const QSize area = viewport()->size();
    // Centred when smaller than the viewport, scrolled when larger.
    const double x = content.width() < area.width() ? (area.width() - content.width()) / 2.0
                                                    : -horizontalScrollBar()->value();
    const double y = content.height() < area.height() ? (area.height() - content.height()) / 2.0
                                                      : -verticalScrollBar()->value();
    return {x, y};
}

const QImage &ImageView::scaledForScreen()
{
    // Reduced once per zoom, with area averaging: a large image drawn small by the painter alone
    // would alias.
    const QSize target = (QSizeF(m_fullSize) * m_zoom).toSize();
    if (target.width() >= m_image.width() || target.height() >= m_image.height()) {
        return m_image;
    }
    if (m_scaled.isNull() || m_scaledFor != target) {
        m_scaled = m_image.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_scaledFor = target;
    }
    return m_scaled;
}

void ImageView::paintEvent(QPaintEvent *)
{
    if (m_image.isNull()) {
        return;
    }
    QPainter painter{viewport()};
    const QImage &image = scaledForScreen();
    // Upscaled smoothly only when the image is a preview: the full image is shown pixel by pixel.
    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_image.size() != m_fullSize);
    painter.drawImage(QRectF{imageOrigin(), QSizeF(m_fullSize) * m_zoom}, image);

    if (m_picking && viewport()->underMouse()) {
        // The area sampled, in black and white, visible on any picture.
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen{Qt::black, 3});
        painter.drawEllipse(m_cursor, PICK_RADIUS, PICK_RADIUS);
        painter.setPen(QPen{Qt::white, 1});
        painter.drawEllipse(m_cursor, PICK_RADIUS, PICK_RADIUS);
    }
}

void ImageView::scrollContentsBy(int, int)
{
    viewport()->update();
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    if (m_fit) {
        m_zoom = fitZoom();
        m_scaled = {};
        Q_EMIT zoomChanged(m_zoom);
    }
    updateScrollBars();
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta != 0) {
        setZoom(m_zoom * std::pow(ZOOM_STEP, delta / 120.0), event->position());
    }
    event->accept();
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (m_picking) {
        if (event->button() == Qt::LeftButton && !m_fullSize.isEmpty()) {
            const QPointF point = (event->position() - imageOrigin()) / m_zoom;
            if (QRectF{QPointF{0, 0}, QSizeF(m_fullSize)}.contains(point)) {
                Q_EMIT pointPicked(point, PICK_RADIUS / m_zoom);
            }
        }
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_panning = true;
        m_panStart = event->position().toPoint();
        m_scrollAtPanStart = {horizontalScrollBar()->value(), verticalScrollBar()->value()};
        viewport()->setCursor(Qt::ClosedHandCursor);
    }
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_picking) {
        m_cursor = event->position();
        viewport()->update();
        return;
    }
    if (m_panning) {
        const QPoint delta = event->position().toPoint() - m_panStart;
        horizontalScrollBar()->setValue(m_scrollAtPanStart.x() - delta.x());
        verticalScrollBar()->setValue(m_scrollAtPanStart.y() - delta.y());
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_panning = false;
        viewport()->setCursor(Qt::OpenHandCursor);
    }
}

void ImageView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_picking) {
        return;
    }
    // As in most viewers: a double click toggles between the whole picture and 100% where
    // clicked.
    if (m_fit) {
        setZoom(1.0, event->position());
    } else {
        fitToWindow();
    }
}

void ImageView::dragEnterEvent(QDragEnterEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.size() == 1 && urls.front().isLocalFile()) {
        event->acceptProposedAction();
    }
}

void ImageView::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.size() == 1 && urls.front().isLocalFile()) {
        event->acceptProposedAction();
        Q_EMIT fileDropped(urls.front().toLocalFile());
    }
}

void ImageView::keyPressEvent(QKeyEvent *event)
{
    if (m_picking && event->key() == Qt::Key_Escape) {
        setPickMode(false);
        Q_EMIT pickCancelled();
        return;
    }
    QAbstractScrollArea::keyPressEvent(event);
}

bool ImageView::viewportEvent(QEvent *event)
{
    // The sampling circle goes when the mouse leaves the picture.
    if (m_picking && event->type() == QEvent::Leave) {
        viewport()->update();
    }
    return QAbstractScrollArea::viewportEvent(event);
}
