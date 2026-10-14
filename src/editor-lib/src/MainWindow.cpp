// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/MainWindow.h"
#include "editor/Document.h"
#include "editor/IconUtils.h"
#include "editor/PreviewRenderer.h"
#include "editor/RecentFilesMenu.h"
#include "editor/SessionProperties.h"
#include "editor/ThemeMenu.h"
#include "editor/actions/BeforeAfterToggleAction.h"
#include "editor/actions/LoadSettingsAction.h"
#include "editor/actions/MatchCameraLookAction.h"
#include "editor/actions/OpenAction.h"
#include "editor/actions/PickWhiteBalanceAction.h"
#include "editor/actions/SaveImageAction.h"
#include "editor/mediators/DialogsMediator.h"
#include "editor/mediators/DocumentPanelMediator.h"
#include "editor/mediators/PreviewMediator.h"
#include "editor/mediators/StatusBarMediator.h"
#include "editor/viewmodel/LeavePictureViewModel.h"
#include "widgets/CurveView.h"
#include "widgets/HistogramView.h"
#include "widgets/ImageView.h"

#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/effects/ColorMixerEffect.h"
#include "core/effects/SharpnessEffect.h"
#include "core/effects/ToneCurveEffect.h"
#include "core/effects/WhiteBalanceEffect.h"

namespace {

/// The pictures listed in File > Open Recent.
constexpr int RECENT_FILES = 10;
constexpr int PANEL_WIDTH = 300;
/// The space beside and above the histogram, in pixels: that of the panel's sections, so they
/// line up.
constexpr int HISTOGRAM_MARGIN = 12;
constexpr int HISTOGRAM_TOP = 8;
/// The space below the eyedropper of the Color section, in pixels, so the slider under it does
/// not touch it.
constexpr int PICK_BOTTOM_SPACE = 6;
/// The size of its icon, in pixels: larger than a tool's, as it is the first thing to use.
constexpr int PICK_ICON_SIZE = 28;

/// How long the status bar tells that a picture was not opened, as another was opening.
constexpr int BUSY_MESSAGE_MS = 5000;

} // namespace

MainWindow::MainWindow(
    Document *document,
    PreviewRenderer *renderer,
    SaveImageViewModel *saveImage,
    CameraLookViewModel *cameraLook,
    LeavePictureViewModel *leavePicture,
    WhiteBalanceViewModel *whiteBalance,
    std::vector<AdjustmentsPanel::Section> sections,
    QWidget *parent)
    : QMainWindow{parent}
    , m_document{document}
    , m_renderer{renderer}
    , m_saveImage{saveImage}
    , m_cameraLook{cameraLook}
    , m_leavePicture{leavePicture}
    , m_whiteBalance{whiteBalance}
    , m_view{new ImageView{this}}
    , m_panel{new AdjustmentsPanel{std::move(sections)}}
    , m_status{new StatusBarMediator{statusBar(), this}}
{
    setCentralWidget(m_view);

    auto *dock = new QDockWidget{tr("Adjustments"), this};
    dock->setObjectName("adjustments");
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    // No title: the sections say what the panel holds.
    dock->setTitleBarWidget(new QWidget{dock});
    // The histogram above the sections, out of their scroll area, so it stays in sight.
    auto *histogram = new HistogramView;
    auto *histogramBox = new QWidget;
    auto *histogramLayout = new QVBoxLayout{histogramBox};
    histogramLayout->setContentsMargins(HISTOGRAM_MARGIN, HISTOGRAM_TOP, HISTOGRAM_MARGIN, 0);
    histogramLayout->addWidget(histogram);
    auto *tools = new QWidget;
    auto *toolsLayout = new QVBoxLayout{tools};
    toolsLayout->setContentsMargins(0, 0, 0, 0);
    toolsLayout->setSpacing(0);
    toolsLayout->addWidget(histogramBox);
    toolsLayout->addWidget(m_panel, 1);
    dock->setWidget(tools);
    m_panel->setMinimumWidth(PANEL_WIDTH);
    // Without a picture, the sliders have nothing to change; the sections still fold and scroll.
    m_panel->setControlsEnabled(false);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    createActions();

    // Every section starts folded: the user opens the tools they need.
    for (const QString &title : m_panel->sectionTitles()) {
        m_panel->setSectionExpanded(title, false);
    }

    // The colour mixer's three groups of sliders, each under its name; and Sharpness and its
    // Radius, one group, under theirs, apart from the texture and clarity above them.
    m_panel->addHeading(core::ColorMixerEffect::hues()[0].id, tr("Hue"));
    m_panel->addHeading(core::ColorMixerEffect::saturations()[0].id, tr("Saturation"));
    m_panel->addHeading(core::ColorMixerEffect::luminances()[0].id, tr("Luminance"));
    m_panel->addHeading(core::SharpnessEffect::SHARPNESS.id, tr("Sharpening"));

    // The curve drawn above the sliders that shape it.
    auto *curveView = new CurveView;
    m_panel->addSectionWidget(QString::fromStdString(core::ToneCurveEffect{}.section()), curveView);

    new DocumentPanelMediator{m_document, m_panel, curveView, m_renderer, this};
    new PreviewMediator{m_renderer, m_view, histogram, this};
    // The status bar first: a failed open gives the cursor back before its warning shows.
    m_status->followDocument(m_document, m_cameraLook);
    m_status->followRenderer(m_renderer);
    m_status->followView(m_view);
    m_status->followSaveImage(m_saveImage);
    m_status->followWhiteBalance(m_whiteBalance);
    m_status->followLeavePicture(m_leavePicture);
    m_status->followCameraLook(m_cameraLook);
    new DialogsMediator{this, m_document, m_leavePicture};

    connect(m_document, &Document::opened, this, &MainWindow::onOpened);
    connect(m_document, &Document::modifiedChanged, this, &MainWindow::setWindowModified);
    connect(m_view, &ImageView::fileDropped, this, &MainWindow::openFile);
    // The window waits for the settings to be written, then closes again, from the event loop.
    connect(
        m_leavePicture,
        &LeavePictureViewModel::readyToClose,
        this,
        &QWidget::close,
        Qt::QueuedConnection);

    // The eyedropper first in the section, above the sliders it sets, larger than a tool's icon:
    // the white balance is the first thing set on a picture. Its tooltip says what it picks.
    auto *pickButton = new QToolButton;
    pickButton->setDefaultAction(m_pickWhite);
    pickButton->setAutoRaise(true);
    pickButton->setIconSize(QSize{PICK_ICON_SIZE, PICK_ICON_SIZE});
    // No padding, so the icon stands at the section's left edge, in line with the sliders' names;
    // the highlight while picking, which the padding's style drew, drawn here.
    pickButton->setStyleSheet(
        "QToolButton { padding: 0px; margin: 0px; border: none; border-radius: 3px; }"
        "QToolButton:hover { background: palette(midlight); }"
        "QToolButton:checked { background: palette(highlight); }");
    auto *pickRow = new QWidget;
    auto *pickLayout = new QHBoxLayout{pickRow};
    pickLayout->setContentsMargins(0, 0, 0, PICK_BOTTOM_SPACE);
    pickLayout->addWidget(pickButton);
    pickLayout->addStretch();
    m_panel->addSectionWidget(QString::fromStdString(core::WhiteBalanceEffect{}.section()), pickRow);

    updateTitle();
    resize(1400, 900);
}

void MainWindow::createActions()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    auto *open = new OpenAction{m_document, this};
    connect(open, &OpenAction::fileChosen, this, &MainWindow::openFile);
    file->addAction(open);

    m_recentFiles = new RecentFilesMenu{tr("Open &Recent"), RECENT_FILES, this};
    m_recentFiles->setFiles(
        QSettings{}.value(SessionProperties::Files::RECENT_FILES).toStringList());
    connect(m_recentFiles, &RecentFilesMenu::fileSelected, this, &MainWindow::openFile);
    connect(m_recentFiles, &RecentFilesMenu::filesChanged, this, [](const QStringList &files) {
        QSettings{}.setValue(SessionProperties::Files::RECENT_FILES, files);
    });
    file->addMenu(m_recentFiles);
    setThemedIcon(open, "document-open");
    QAction *save
        = file->addAction(tr("&Save Settings"), QKeySequence::Save, m_document, &Document::save);
    setThemedIcon(save, "document-save");
    QAction *loadSettings = new LoadSettingsAction{m_document, this};
    file->addAction(loadSettings);
    file->addSeparator();
    auto *saveImageAction = new SaveImageAction{m_saveImage, this};
    connect(saveImageAction, &SaveImageAction::started, m_status, &StatusBarMediator::showSaving);
    file->addAction(saveImageAction);
    file->addSeparator();
    file->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    // The way back from a change saved a second later, e.g. settings loaded by mistake: the grade
    // the picture had when it was opened.
    QAction *revert = edit->addAction(tr("&Revert to Opened"), m_document, &Document::revertSettings);
    revert->setToolTip(tr("Go back to the settings the picture had when it was opened"));
    QAction *reset = edit->addAction(
        tr("Revert to &Default"),
        QKeySequence{tr("Ctrl+Shift+R")},
        m_document,
        &Document::resetSettings);
    reset->setToolTip(tr("Put every slider back to its default: no edit"));
    edit->addSeparator();
    // Neither in a menu nor on the toolbar: its button is at the top of the Color section. The
    // window holds it, so its key works anywhere in the window.
    m_pickWhite = new PickWhiteBalanceAction{m_whiteBalance, m_view, this};
    addAction(m_pickWhite);
    setThemedIcon(m_pickWhite, "color-picker");
    QAction *matchLook = new MatchCameraLookAction{m_cameraLook, this};
    edit->addAction(matchLook);

    QMenu *view = menuBar()->addMenu(tr("&View"));
    auto *showBefore = new BeforeAfterToggleAction{m_document, m_renderer, this};
    connect(showBefore, &QAction::toggled, m_status, &StatusBarMediator::showRendering);
    connect(
        showBefore, &BeforeAfterToggleAction::beforeShown, m_status, &StatusBarMediator::showBefore);
    view->addAction(showBefore);
    setThemedIcon(showBefore, "image-v-split");
    view->addSeparator();
    QAction *zoomIn = view->addAction(
        IconUtils::themedIcon("zoom-in"),
        tr("Zoom &In"),
        QKeySequence::ZoomIn,
        m_view,
        &ImageView::zoomIn);
    QAction *zoomOut = view->addAction(
        IconUtils::themedIcon("zoom-out"),
        tr("Zoom &Out"),
        QKeySequence::ZoomOut,
        m_view,
        &ImageView::zoomOut);
    QAction *fit = view->addAction(
        IconUtils::themedIcon("zoom-fit-best"),
        tr("&Fit to Window"),
        QKeySequence{tr("Ctrl+0")},
        m_view,
        &ImageView::fitToWindow);
    QAction *actual = view->addAction(
        IconUtils::themedIcon("zoom-original"),
        tr("&Actual Size"),
        QKeySequence{tr("Ctrl+1")},
        m_view,
        &ImageView::actualSize);

    setThemedIcon(zoomIn, "zoom-in");
    setThemedIcon(zoomOut, "zoom-out");
    setThemedIcon(fit, "zoom-fit-best");
    setThemedIcon(actual, "zoom-original");

    createSettingsMenu();

    // The actions used most, one click away: fixed, without a menu of its own.
    QToolBar *toolbar = addToolBar(tr("Tools"));
    toolbar->setObjectName("tools");
    toolbar->setMovable(false);
    toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    toolbar->addAction(open);
    toolbar->addAction(save);
    toolbar->addSeparator();
    toolbar->addAction(showBefore);
    toolbar->addSeparator();
    for (QAction *action : {zoomIn, zoomOut, fit, actual}) {
        toolbar->addAction(action);
    }

    for (QAction *action : std::initializer_list<QAction *>{
             save, loadSettings, saveImageAction, revert, reset, m_pickWhite, showBefore, matchLook}) {
        action->setEnabled(false);
        connect(m_document, &Document::opened, action, [action] { action->setEnabled(true); });
    }
}

void MainWindow::openFile(const QString &path)
{
    const auto opening = m_leavePicture->open(path);
    if (opening) {
        return;
    }
    switch (opening.error()) {
    case LeavePictureViewModel::Refusal::Missing:
        // E.g. a recent picture since moved or deleted: it goes from the list.
        m_recentFiles->removeFile(path);
        QMessageBox::warning(
            this, tr("Cannot Open"), tr("%1 no longer exists.").arg(QDir::toNativeSeparators(path)));
        break;
    case LeavePictureViewModel::Refusal::NotAPicture:
        QMessageBox::warning(
            this,
            tr("Cannot Open"),
            tr("%1 is not a TIFF, DNG or camera raw file.").arg(QFileInfo{path}.fileName()));
        break;
    case LeavePictureViewModel::Refusal::Busy:
        statusBar()->showMessage(
            tr("%1 is not opened: wait for the picture opening first.")
                .arg(QFileInfo{path}.fileName()),
            BUSY_MESSAGE_MS);
        break;
    }
}

void MainWindow::onOpened()
{
    m_recentFiles->addFile(m_document->path());
    updateTitle();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_leavePicture->close()) {
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

void MainWindow::createSettingsMenu()
{
    QMenu *settings = menuBar()->addMenu(tr("&Settings"));
    auto *themes = new ThemeMenu{new QSettings{this}, settings};
    connect(themes, &ThemeMenu::themeChanged, this, &MainWindow::refreshIcons);
    settings->addMenu(themes);
}

void MainWindow::setThemedIcon(QAction *action, const QString &name)
{
    action->setIcon(IconUtils::themedIcon(name));
    m_themedIcons.emplace_back(action, name);
}

void MainWindow::refreshIcons()
{
    // An icon keeps the drawing of the theme it was made in: each is made again for the new one.
    for (const auto &[action, name] : m_themedIcons) {
        action->setIcon(IconUtils::themedIcon(name));
    }
}
