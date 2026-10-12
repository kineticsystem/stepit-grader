// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/MainWindow.h"
#include "editor/Document.h"
#include "editor/Icons.h"
#include "editor/PreviewRenderer.h"
#include "editor/Theme.h"
#include "editor/WhiteBalancePicker.h"
#include "editor/widgets/ImageView.h"
#include "editor/widgets/RecentFilesMenu.h"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

#include "core/effects/WhiteBalanceEffect.h"

namespace {

constexpr const char *LAST_FOLDER_KEY = "lastFolder";
constexpr const char *RECENT_FILES_KEY = "recentPictures";
/// The pictures listed in File > Open Recent.
constexpr int RECENT_FILES = 10;
constexpr int PANEL_WIDTH = 300;
/// How long the status bar shows that the settings were saved, which happens after every edit.
constexpr int SAVED_MESSAGE_MS = 3000;
/// How long it shows a failed save; the title's * stays until a save succeeds.
constexpr int SAVE_FAILED_MESSAGE_MS = 15000;

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
    , m_beforeLabel{new QLabel{tr("Before"), this}}
{
    setCentralWidget(m_view);

    auto *dock = new QDockWidget{tr("Adjustments"), this};
    dock->setObjectName("adjustments");
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setWidget(m_panel);
    m_panel->setMinimumWidth(PANEL_WIDTH);
    // Without a picture, the sliders have nothing to change; the sections still fold and scroll.
    m_panel->setControlsEnabled(false);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    statusBar()->addWidget(m_fileLabel, 1);
    statusBar()->addPermanentWidget(m_beforeLabel);
    statusBar()->addPermanentWidget(m_renderLabel);
    QFont bold = m_beforeLabel->font();
    bold.setBold(true);
    m_beforeLabel->setFont(bold);
    m_beforeLabel->hide();
    statusBar()->addPermanentWidget(m_zoomLabel);

    createActions();

    // Every section starts folded: the user opens the tools they need.
    for (const QString &title : m_panel->sectionTitles()) {
        m_panel->setSectionExpanded(title, false);
    }

    connect(m_panel, &AdjustmentsPanel::settingsChanged, m_document, &Document::setSettings);
    connect(m_document, &Document::settingsChanged, this, [this](const core::Settings &settings) {
        m_panel->setSettings(settings);
        // A change is seen on the edited picture: the picture before the edit gives way to it.
        if (m_showBefore->isChecked()) {
            const QSignalBlocker blocker{m_showBefore};
            m_showBefore->setChecked(false);
            m_beforeLabel->hide();
        }
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
        statusBar()
            ->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(path)), SAVED_MESSAGE_MS);
    });
    // The document saves by itself and tries again at the next change: a dialog for each failure
    // would interrupt every slider move.
    connect(m_document, &Document::saveFailed, this, [this](const QString &message) {
        statusBar()
            ->showMessage(tr("Cannot save the settings: %1").arg(message), SAVE_FAILED_MESSAGE_MS);
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

    m_recentFiles = new RecentFilesMenu{tr("Open &Recent"), RECENT_FILES, this};
    m_recentFiles->setFiles(QSettings{}.value(RECENT_FILES_KEY).toStringList());
    connect(m_recentFiles, &RecentFilesMenu::fileSelected, this, &MainWindow::openFile);
    connect(m_recentFiles, &RecentFilesMenu::filesChanged, this, [](const QStringList &files) {
        QSettings{}.setValue(RECENT_FILES_KEY, files);
    });
    file->addMenu(m_recentFiles);
    QAction *open = file->actions().constFirst();
    setThemedIcon(open, "document-open");
    QAction *save
        = file->addAction(tr("&Save Settings"), QKeySequence::Save, m_document, &Document::save);
    setThemedIcon(save, "document-save");
    file->addSeparator();
    file->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    QAction *revert = edit->addAction(tr("&Revert to Opened"), m_document, &Document::revertSettings);
    QAction *reset = edit->addAction(
        tr("Reset &All"), QKeySequence{tr("Ctrl+Shift+R")}, m_document, &Document::resetSettings);
    edit->addSeparator();
    m_pickWhite = edit->addAction(tr("Pick &White Balance"));
    setThemedIcon(m_pickWhite, "color-picker");
    m_pickWhite->setShortcut(QKeySequence{tr("W")});
    m_pickWhite->setCheckable(true);
    m_pickWhite->setToolTip(tr("Click an area that should be white or grey (W)"));
    connect(m_pickWhite, &QAction::toggled, m_view, &ImageView::setPickMode);

    QMenu *view = menuBar()->addMenu(tr("&View"));
    // Lightroom's key for it.
    m_showBefore = view->addAction(tr("&Before and After"));
    setThemedIcon(m_showBefore, "image-v-split");
    m_showBefore->setShortcut(QKeySequence{Qt::Key_Backslash});
    m_showBefore->setCheckable(true);
    m_showBefore->setToolTip(tr("Show the picture before the edit, or after (\\)"));
    connect(m_showBefore, &QAction::toggled, this, &MainWindow::showBefore);
    view->addSeparator();
    QAction *zoomIn = view->addAction(
        themedIcon("zoom-in"), tr("Zoom &In"), QKeySequence::ZoomIn, m_view, &ImageView::zoomIn);
    QAction *zoomOut = view->addAction(
        themedIcon("zoom-out"), tr("Zoom &Out"), QKeySequence::ZoomOut, m_view, &ImageView::zoomOut);
    QAction *fit = view->addAction(
        themedIcon("zoom-fit-best"),
        tr("&Fit to Window"),
        QKeySequence{tr("Ctrl+0")},
        m_view,
        &ImageView::fitToWindow);
    QAction *actual = view->addAction(
        themedIcon("zoom-original"),
        tr("&Actual Size"),
        QKeySequence{tr("Ctrl+1")},
        m_view,
        &ImageView::actualSize);

    setThemedIcon(zoomIn, "zoom-in");
    setThemedIcon(zoomOut, "zoom-out");
    setThemedIcon(fit, "zoom-fit-best");
    setThemedIcon(actual, "zoom-original");

    createSettingsMenu();

    // The actions used most, one click away, as in Lentigram: fixed, without a menu of its own.
    QToolBar *toolbar = addToolBar(tr("Tools"));
    toolbar->setObjectName("tools");
    toolbar->setMovable(false);
    toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    toolbar->addAction(open);
    toolbar->addAction(save);
    toolbar->addSeparator();
    toolbar->addAction(m_showBefore);
    toolbar->addAction(m_pickWhite);
    toolbar->addSeparator();
    for (QAction *action : {zoomIn, zoomOut, fit, actual}) {
        toolbar->addAction(action);
    }

    for (QAction *action : {save, revert, reset, m_pickWhite, m_showBefore}) {
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
    if (!QFileInfo::exists(path)) {
        // E.g. a recent picture since moved or deleted: it goes from the list.
        m_recentFiles->removeFile(path);
        QMessageBox::warning(
            this, tr("Cannot Open"), tr("%1 no longer exists.").arg(QDir::toNativeSeparators(path)));
        return;
    }
    if (!m_document->canOpen(path)) {
        QMessageBox::warning(
            this,
            tr("Cannot Open"),
            tr("%1 is not a TIFF, DNG or camera raw file.").arg(QFileInfo{path}.fileName()));
        return;
    }
    if (m_document->isOpening() || !settingsSaved()) {
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_fileLabel->setText(tr("Opening %1…").arg(QDir::toNativeSeparators(path)));
    m_document->open(path);
}

void MainWindow::onOpened()
{
    QApplication::restoreOverrideCursor();
    m_recentFiles->addFile(m_document->path());
    const auto image = m_document->image();
    m_renderer->setSource(image);
    m_panel->setControlsEnabled(true);
    m_fileLabel->setText(tr("%1 — %2 × %3")
                             .arg(QDir::toNativeSeparators(m_document->path()))
                             .arg(image->width)
                             .arg(image->height));
    updateTitle();
    // The settings are rendered once the document announces them, right after.
}

bool MainWindow::settingsSaved()
{
    if (m_document->save()) {
        return true;
    }
    // Only here would the edit be lost, so only here does a failure ask.
    const auto answer = QMessageBox::warning(
        this,
        tr("Cannot Save"),
        tr("The settings of %1 cannot be saved. Leave them unsaved?")
            .arg(QFileInfo{m_document->path()}.fileName()),
        QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Cancel);
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (settingsSaved()) {
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

void MainWindow::showBefore(bool before)
{
    m_beforeLabel->setVisible(before);
    // The picture before the edit is the picture with no settings: the renderer shows it from its
    // cache once rendered, so switching back and forth is at once.
    m_renderLabel->setText(tr("Rendering…"));
    m_renderer->render(before ? core::Settings{} : m_document->settings());
}

void MainWindow::createSettingsMenu()
{
    QMenu *settings = menuBar()->addMenu(tr("&Settings"));
    QMenu *themes = settings->addMenu(tr("&Theme"));
    auto *group = new QActionGroup{themes};
    const Theme current = themeNamed(QSettings{}.value(THEME_SETTING).toString())
                              .value_or(Theme::System);
    const std::pair<Theme, QString> choices[]
        = {{Theme::System, tr("&System")}, {Theme::Light, tr("&Light")}, {Theme::Dark, tr("&Dark")}};
    for (const auto &[theme, label] : choices) {
        QAction *action = themes->addAction(label);
        action->setCheckable(true);
        action->setChecked(theme == current);
        group->addAction(action);
        connect(action, &QAction::triggered, this, [this, theme] {
            applyTheme(theme);
            QSettings{}.setValue(THEME_SETTING, nameOf(theme));
            refreshIcons();
        });
    }
}

void MainWindow::setThemedIcon(QAction *action, const QString &name)
{
    action->setIcon(themedIcon(name));
    m_themedIcons.emplace_back(action, name);
}

void MainWindow::refreshIcons()
{
    // An icon keeps the drawing of the theme it was made in: each is made again for the new one.
    for (const auto &[action, name] : m_themedIcons) {
        action->setIcon(themedIcon(name));
    }
}
