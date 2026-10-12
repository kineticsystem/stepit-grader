// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/MainWindow.h"
#include "editor/Document.h"
#include "editor/Icons.h"
#include "editor/PreviewRenderer.h"
#include "editor/WhiteBalancePicker.h"
#include "editor/widgets/ImageView.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>

#include "core/effects/WhiteBalanceEffect.h"

namespace {

constexpr const char *LAST_FOLDER_KEY = "lastFolder";
/// Followed by a section's title: whether it is closed.
constexpr const char *COLLAPSED_KEY = "collapsedSections/";
constexpr int PANEL_WIDTH = 300;

} // namespace

MainWindow::MainWindow(
    Document *document,
    PreviewRenderer *renderer,
    std::vector<AdjustmentsPanel::Section> sections,
    QWidget *parent)
    : QMainWindow{parent}
    , m_document{document}
    , m_renderer{renderer}
    , m_view{new ImageView{this}}
    , m_panel{new AdjustmentsPanel{std::move(sections)}}
    , m_fileLabel{new QLabel{this}}
    , m_zoomLabel{new QLabel{this}}
    , m_renderLabel{new QLabel{this}}
{
    setCentralWidget(m_view);

    auto *dock = new QDockWidget{tr("Adjustments"), this};
    dock->setObjectName("adjustments");
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setWidget(m_panel);
    m_panel->setMinimumWidth(PANEL_WIDTH);
    m_panel->setEnabled(false);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    statusBar()->addWidget(m_fileLabel, 1);
    statusBar()->addPermanentWidget(m_renderLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);

    createActions();

    // The sections closed stay closed the next time.
    const QSettings settings;
    for (const QString &title : m_panel->sectionTitles()) {
        m_panel->setSectionExpanded(title, !settings.value(COLLAPSED_KEY + title, false).toBool());
    }
    connect(
        m_panel,
        &AdjustmentsPanel::sectionExpandedChanged,
        this,
        [](const QString &title, bool expanded) {
            QSettings{}.setValue(COLLAPSED_KEY + title, !expanded);
        });

    connect(m_panel, &AdjustmentsPanel::settingsChanged, m_document, &Document::setSettings);
    connect(m_document, &Document::settingsChanged, this, [this](const core::Settings &settings) {
        m_panel->setSettings(settings);
        m_renderLabel->setText(tr("Rendering…"));
        m_renderer->render(settings);
    });
    connect(m_document, &Document::opened, this, &MainWindow::onOpened);
    connect(m_document, &Document::openFailed, this, [this](const QString &message) {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, tr("Cannot Open"), message);
    });
    connect(m_document, &Document::modifiedChanged, this, &MainWindow::setWindowModified);
    connect(m_document, &Document::saved, this, [this](const QString &path) {
        statusBar()->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(path)), 5000);
    });
    connect(m_document, &Document::saveFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Cannot Save"), message);
    });
    connect(
        m_renderer,
        &PreviewRenderer::rendered,
        this,
        [this](const QImage &image, QSize fullSize, bool full) {
            m_view->setImage(image, fullSize);
            m_renderLabel->setText(full ? QString{} : tr("Preview"));
        });
    connect(m_view, &ImageView::fileDropped, this, &MainWindow::openFile);
    connect(m_view, &ImageView::zoomChanged, this, &MainWindow::updateZoom);
    connect(m_view, &ImageView::pointPicked, this, &MainWindow::pickWhiteBalanceAt);
    connect(m_view, &ImageView::pickCancelled, m_pickWhite, [this] {
        m_pickWhite->setChecked(false);
    });

    // The eyedropper next to the sliders it sets.
    auto *pickButton = new QToolButton;
    pickButton->setDefaultAction(m_pickWhite);
    pickButton->setAutoRaise(true);
    m_panel->addSectionHeaderWidget(
        QString::fromStdString(core::WhiteBalanceEffect{}.section()), pickButton);

    updateTitle();
    resize(1400, 900);
}

void MainWindow::createActions()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&Open…"), QKeySequence::Open, this, &MainWindow::chooseFile);
    QAction *save
        = file->addAction(tr("&Save Settings"), QKeySequence::Save, m_document, &Document::save);
    file->addSeparator();
    file->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    QAction *revert = edit->addAction(tr("&Revert to Saved"), m_document, &Document::revertSettings);
    QAction *reset = edit->addAction(
        tr("Reset &All"), QKeySequence{tr("Ctrl+Shift+R")}, m_document, &Document::resetSettings);
    edit->addSeparator();
    m_pickWhite = edit->addAction(eyedropperIcon(), tr("Pick &White Balance"));
    m_pickWhite->setShortcut(QKeySequence{tr("W")});
    m_pickWhite->setCheckable(true);
    m_pickWhite->setToolTip(tr("Click an area that should be white or grey (W)"));
    connect(m_pickWhite, &QAction::toggled, m_view, &ImageView::setPickMode);

    QMenu *view = menuBar()->addMenu(tr("&View"));
    view->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, m_view, &ImageView::zoomIn);
    view->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, m_view, &ImageView::zoomOut);
    view->addAction(tr("&Fit to Window"), QKeySequence{tr("Ctrl+0")}, m_view, &ImageView::fitToWindow);
    view->addAction(tr("&Actual Size"), QKeySequence{tr("Ctrl+1")}, m_view, &ImageView::actualSize);

    for (QAction *action : {save, revert, reset, m_pickWhite}) {
        action->setEnabled(false);
        connect(m_document, &Document::opened, action, [action] { action->setEnabled(true); });
    }
}

void MainWindow::chooseFile()
{
    QSettings settings;
    const QString filter = tr("Images (%1)").arg(m_document->nameFilters().join(' '));
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Image"), settings.value(LAST_FOLDER_KEY).toString(), filter);
    if (!path.isEmpty()) {
        settings.setValue(LAST_FOLDER_KEY, QFileInfo{path}.absolutePath());
        openFile(path);
    }
}

void MainWindow::openFile(const QString &path)
{
    if (!m_document->canOpen(path)) {
        QMessageBox::warning(
            this,
            tr("Cannot Open"),
            tr("%1 is not a TIFF, DNG or camera raw file.").arg(QFileInfo{path}.fileName()));
        return;
    }
    if (m_document->isOpening() || !maybeSave()) {
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_fileLabel->setText(tr("Opening %1…").arg(QDir::toNativeSeparators(path)));
    m_document->open(path);
}

void MainWindow::onOpened()
{
    QApplication::restoreOverrideCursor();
    const auto image = m_document->image();
    m_renderer->setSource(image);
    m_panel->setEnabled(true);
    m_fileLabel->setText(tr("%1 — %2 × %3")
                             .arg(QDir::toNativeSeparators(m_document->path()))
                             .arg(image->width)
                             .arg(image->height));
    updateTitle();
    // The settings are rendered once the document announces them, right after.
}

bool MainWindow::maybeSave()
{
    if (!m_document->isModified()) {
        return true;
    }
    const auto answer = QMessageBox::question(
        this,
        tr("Save Settings"),
        tr("Save the settings of %1?").arg(QFileInfo{m_document->path()}.fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) {
        return m_document->save();
    }
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::updateTitle()
{
    const QString name = m_document->isOpen() ? QFileInfo{m_document->path()}.fileName()
                                              : tr("No Image");
    setWindowTitle(tr("%1[*] — StepIt Grader").arg(name));
    setWindowModified(m_document->isModified());
}

void MainWindow::pickWhiteBalanceAt(QPointF point, double radius)
{
    // One pick, then back to dragging, as most editors do.
    m_pickWhite->setChecked(false);
    if (!m_document->isOpen()) {
        return;
    }
    const auto pick = pickWhiteBalance(*m_document->image(), m_document->settings(), point, radius);
    if (!pick) {
        statusBar()->showMessage(pick.error(), 6000);
        return;
    }
    m_document->setSettings(pick->settings);
    QString message = pick->clamped ? tr("White balance set as near neutral as the sliders go.")
                                    : tr("White balance set from the area picked.");
    if (pick->clipped > 0) {
        message += ' ' + tr("%n clipped pixel(s) left out.", nullptr, pick->clipped);
    }
    statusBar()->showMessage(message, 6000);
}

void MainWindow::updateZoom(double zoom)
{
    m_zoomLabel->setText(QString{"%1%"}.arg(qRound(zoom * 100)));
}
