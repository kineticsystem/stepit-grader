// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/MainWindow.h"
#include "editor/CameraLook.h"
#include "editor/Document.h"
#include "editor/Icons.h"
#include "editor/ImageExport.h"
#include "editor/PreviewRenderer.h"
#include "editor/RecentFilesMenu.h"
#include "editor/Theme.h"
#include "editor/WaitingDialog.h"
#include "editor/WhiteBalancePicker.h"
#include "imageio/RawReader.h"
#include "widgets/CurveView.h"
#include "widgets/HistogramView.h"
#include "widgets/ImageView.h"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "core/effects/ColorMixerEffect.h"
#include "core/effects/ToneCurveEffect.h"
#include "core/effects/WhiteBalanceEffect.h"

namespace {

constexpr const char *LAST_FOLDER_KEY = "lastFolder";
/// Where settings were last loaded from: often not where the pictures are.
constexpr const char *LAST_SETTINGS_FOLDER_KEY = "lastSettingsFolder";
constexpr const char *RECENT_FILES_KEY = "recentPictures";
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
/// How long the status bar shows that the settings were saved, which happens after every edit.
constexpr int SAVED_MESSAGE_MS = 3000;
/// How long it shows a failed save; the title's * stays until a save succeeds.
constexpr int SAVE_FAILED_MESSAGE_MS = 15000;
/// The format the picture was last saved as, by its filter: the next save offers it first.
constexpr const char *IMAGE_FORMAT_KEY = "imageFormat";

/// How long the status bar tells that a raw file can be matched to its camera's look.
constexpr int RAW_HINT_MS = 10000;

/// Whether the file is a camera raw file or a DNG, which holds its camera's JPEG.
bool isRawFile(const QString &path)
{
    const std::string suffix = QFileInfo{path}.suffix().toLower().toStdString();
    const auto extensions = imageio::RawReader{}.extensions();
    return std::ranges::find(extensions, suffix) != extensions.end();
}

/// The points the curve of the Tone Curve section is drawn with: enough for it to look smooth.
constexpr int CURVE_POINTS = 129;

/// Draws the tone curve the settings make, with the borders of its regions.
void showCurve(CurveView &view, const core::Settings &settings)
{
    const auto knots = core::ToneCurveEffect::knots(settings);
    std::vector<QPointF> points;
    points.reserve(CURVE_POINTS);
    for (int i = 0; i < CURVE_POINTS; ++i) {
        const double tone = static_cast<double>(i) / (CURVE_POINTS - 1);
        points.emplace_back(tone, core::ToneCurveEffect::curve(knots, tone));
    }
    const auto splits = core::ToneCurveEffect::splits(settings);
    view.setCurve(std::move(points), {splits.begin(), splits.end()});
}

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

    // The colour mixer's three groups of sliders, each under its name.
    m_panel->addHeading(core::ColorMixerEffect::hues()[0].id, tr("Hue"));
    m_panel->addHeading(core::ColorMixerEffect::saturations()[0].id, tr("Saturation"));
    m_panel->addHeading(core::ColorMixerEffect::luminances()[0].id, tr("Luminance"));

    // The curve drawn above the sliders that shape it.
    auto *curveView = new CurveView;
    showCurve(*curveView, core::Settings{});
    m_panel->addSectionWidget(QString::fromStdString(core::ToneCurveEffect{}.section()), curveView);

    connect(m_panel, &AdjustmentsPanel::settingsChanged, m_document, &Document::setSettings);
    connect(
        m_document,
        &Document::settingsChanged,
        this,
        [this, curveView](const core::Settings &settings) {
            m_panel->setSettings(settings);
            showCurve(*curveView, settings);
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
    connect(m_document, &Document::settingsLoaded, this, [this](const QString &file) {
        statusBar()
            ->showMessage(tr("Loaded the settings of %1").arg(QDir::toNativeSeparators(file)), 5000);
    });
    connect(m_document, &Document::sidecarUnreadable, this, [this](const QString &message) {
        QMessageBox::warning(
            this,
            tr("Cannot Read the Settings"),
            tr("%1\n\nThe picture opens with no edit. The first change replaces the settings "
               "file, and keeps the damaged one beside it, with .damaged after its name.")
                .arg(message));
    });
    connect(m_document, &Document::settingsLoadFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Cannot Load Settings"), message);
    });
    connect(m_document, &Document::openFailed, this, [this](const QString &message) {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, tr("Cannot Open"), message);
    });
    connect(m_document, &Document::modifiedChanged, this, &MainWindow::setWindowModified);
    connect(m_document, &Document::saved, this, [this](const QString &path) {
        statusBar()
            ->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(path)), SAVED_MESSAGE_MS);
        continueWhenSaved(false);
    });
    // The document saves by itself and tries again at the next change: a dialog for each failure
    // would interrupt every slider move.
    connect(m_document, &Document::saveFailed, this, [this](const QString &message) {
        statusBar()
            ->showMessage(tr("Cannot save the settings: %1").arg(message), SAVE_FAILED_MESSAGE_MS);
        continueWhenSaved(true);
    });
    connect(
        m_renderer,
        &PreviewRenderer::rendered,
        this,
        [this, histogram](const QImage &image, QSize fullSize, bool full) {
            m_view->setImage(image, fullSize);
            histogram->setHistogram(histogramOf(image));
            m_renderLabel->setText(full ? QString{} : tr("Preview"));
        });
    connect(m_view, &ImageView::fileDropped, this, &MainWindow::openFile);
    connect(m_view, &ImageView::zoomChanged, this, &MainWindow::updateZoom);
    connect(m_view, &ImageView::pointPicked, this, &MainWindow::pickWhiteBalanceAt);
    connect(m_view, &ImageView::pickCancelled, m_pickWhite, [this] {
        m_pickWhite->setChecked(false);
    });

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
    QAction *loadSettings = file->addAction(
        tr("&Load Settings…"), QKeySequence{tr("Ctrl+L")}, this, &MainWindow::chooseSettings);
    loadSettings->setToolTip(
        tr("Apply the settings of a sidecar, e.g. one saved for another picture"));
    file->addSeparator();
    QAction *saveImage = file->addAction(
        tr("Save &Image…"), QKeySequence{tr("Ctrl+Shift+E")}, this, &MainWindow::chooseImageFile);
    saveImage->setToolTip(tr("Save the picture with its grade, at its full size"));
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
    m_pickWhite = new QAction{tr("Pick &White Balance"), this};
    addAction(m_pickWhite);
    setThemedIcon(m_pickWhite, "color-picker");
    m_pickWhite->setShortcut(QKeySequence{tr("W")});
    m_pickWhite->setCheckable(true);
    m_pickWhite->setToolTip(tr("Click an area that should be white or grey (W)"));
    connect(m_pickWhite, &QAction::toggled, m_view, &ImageView::setPickMode);
    QAction *matchLook
        = edit->addAction(tr("&Match Camera Look…"), this, &MainWindow::matchCameraLook);
    matchLook->setToolTip(tr("Find the grade that makes raw files look like their camera's JPEGs"));

    QMenu *view = menuBar()->addMenu(tr("&View"));
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

    // The actions used most, one click away: fixed, without a menu of its own.
    QToolBar *toolbar = addToolBar(tr("Tools"));
    toolbar->setObjectName("tools");
    toolbar->setMovable(false);
    toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    toolbar->addAction(open);
    toolbar->addAction(save);
    toolbar->addSeparator();
    toolbar->addAction(m_showBefore);
    toolbar->addSeparator();
    for (QAction *action : {zoomIn, zoomOut, fit, actual}) {
        toolbar->addAction(action);
    }

    for (QAction *action :
         {save, loadSettings, saveImage, revert, reset, m_pickWhite, m_showBefore, matchLook}) {
        action->setEnabled(false);
        connect(m_document, &Document::opened, action, [action] { action->setEnabled(true); });
    }
}

void MainWindow::chooseImageFile()
{
    const auto &formats = imageFormats();
    QStringList filters;
    for (const ImageFormat &format : formats) {
        filters << format.filter;
    }
    QString selected = QSettings{}.value(IMAGE_FORMAT_KEY).toString();
    if (!filters.contains(selected)) {
        selected = filters.front();
    }
    const auto formatOf = [&formats](const QString &filter) {
        return *std::ranges::find(formats, filter, &ImageFormat::filter);
    };
    const QFileInfo picture{m_document->path()};
    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Save Image"),
        picture.dir().filePath(picture.completeBaseName() + "-graded." + formatOf(selected).suffix),
        filters.join(";;"),
        &selected);
    if (path.isEmpty()) {
        return;
    }
    const ImageFormat format = formatOf(filters.contains(selected) ? selected : filters.front());
    QSettings{}.setValue(IMAGE_FORMAT_KEY, format.filter);
    if (QFileInfo{path}.suffix().isEmpty()) {
        path += "." + format.suffix;
    }
    if (QFileInfo{path}.absoluteFilePath() == picture.absoluteFilePath()) {
        QMessageBox::warning(
            this,
            tr("Save Image"),
            tr("The picture would replace the one it was made from: choose another name."));
        return;
    }

    statusBar()->showMessage(tr("Saving %1…").arg(QDir::toNativeSeparators(path)));
    using Outcome = std::expected<void, QString>;
    auto *watcher = new QFutureWatcher<Outcome>{this};
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, path] {
        watcher->deleteLater();
        const Outcome outcome = watcher->result();
        if (outcome) {
            statusBar()
                ->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(path)), SAVED_MESSAGE_MS);
        } else {
            statusBar()->clearMessage();
            QMessageBox::warning(this, tr("Cannot Save the Image"), outcome.error());
        }
    });
    watcher->setFuture(QtConcurrent::run(
        [image = m_document->image(),
         settings = m_document->settings(),
         pipeline = m_renderer->pipeline(),
         path,
         format] { return saveImage(*image, settings, *pipeline, path, format); }));
}

void MainWindow::matchCameraLook()
{
    // A raw file holds its camera's JPEG: it is matched itself. A TIFF does not: the raw files it
    // was made from are.
    if (isRawFile(m_document->path())) {
        runCameraLook({m_document->path()});
        return;
    }
    QStringList patterns;
    for (const std::string &extension : imageio::RawReader{}.extensions()) {
        const QString suffix = QString::fromStdString(extension);
        patterns << "*." + suffix << "*." + suffix.toUpper();
    }
    const QStringList raws = QFileDialog::getOpenFileNames(
        this,
        tr("Choose the Raw Files This Picture Was Made From"),
        QFileInfo{m_document->path()}.absolutePath(),
        tr("Camera raw files (%1)").arg(patterns.join(' ')));
    if (!raws.isEmpty()) {
        runCameraLook(raws);
    }
}

void MainWindow::runCameraLook(const QStringList &raws)
{
    auto controller = std::make_shared<StatusController>();
    auto *dialog = new WaitingDialog{controller, tr("Stop"), this};
    dialog->setWindowTitle(tr("Matching the Camera's Look"));
    using Outcome = std::expected<core::match::Result, QString>;
    auto *watcher = new QFutureWatcher<Outcome>{this};
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, dialog] {
        dialog->accept();
        dialog->deleteLater();
        watcher->deleteLater();
        showCameraLook(watcher->result());
    });
    watcher->setFuture(QtConcurrent::run([raws, pipeline = m_renderer->pipeline(), controller] {
        return ::matchCameraLook(raws, *pipeline, *controller);
    }));
    dialog->open();
}

void MainWindow::showCameraLook(const std::expected<core::match::Result, QString> &result)
{
    const QString title = tr("Match Camera Look");
    if (!result) {
        QMessageBox::warning(this, title, result.error());
        return;
    }
    QString text = tr("The difference from the camera's JPEG is %1 before any edit, and %2 with "
                      "the grade found, after %3 trials.")
                       .arg(result->errorBefore, 0, 'f', 1)
                       .arg(result->errorAfter, 0, 'f', 1)
                       .arg(result->trials);
    if (result->stopped) {
        text += "\n\n" + tr("The search was stopped: this is the best grade found by then.");
    }
    text += "\n\n"
            + tr(
                "Apply it to the picture? It sets the white balance, the tone, the tone curve and "
                "the colour mixer; the other sliders stay as they are. The picture's settings keep "
                "it, as any grade.");
    if (QMessageBox::question(
            this, title, text, QMessageBox::Apply | QMessageBox::Discard, QMessageBox::Apply)
        != QMessageBox::Apply) {
        return;
    }
    core::Settings settings = m_document->settings();
    for (const auto &[id, value] : result->settings.values()) {
        settings.setValue(id, value);
    }
    m_document->setSettings(settings);
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
    if (m_document->isOpening()) {
        return;
    }
    whenSaved([this, path] {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        m_fileLabel->setText(tr("Opening %1…").arg(QDir::toNativeSeparators(path)));
        m_document->open(path);
    });
}

void MainWindow::onOpened()
{
    QApplication::restoreOverrideCursor();
    m_recentFiles->addFile(m_document->path());
    const auto image = m_document->image();
    m_renderer->setSource(image);
    m_panel->setControlsEnabled(true);
    if (isRawFile(m_document->path())) {
        statusBar()->showMessage(
            tr("A raw file: Edit > Match Camera Look grades it like its camera's JPEG"),
            RAW_HINT_MS);
    }
    m_fileLabel->setText(tr("%1 — %2 × %3")
                             .arg(QDir::toNativeSeparators(m_document->path()))
                             .arg(image->width)
                             .arg(image->height));
    updateTitle();
    // The settings are rendered once the document announces them, right after.
}

void MainWindow::whenSaved(std::function<void()> then)
{
    m_whenSaved = std::move(then);
    m_document->save();
    continueWhenSaved(false);
}

void MainWindow::continueWhenSaved(bool failed)
{
    // A save asked before may end first: the action waits for the last one.
    if (!m_whenSaved || m_document->isSaving()) {
        return;
    }
    if (m_document->isModified()) {
        if (!failed) {
            // Changed again while it was written: written again.
            m_document->save();
            if (m_document->isSaving()) {
                return;
            }
        }
        // Only here would the edit be lost, so only here does a failure ask.
        const auto answer = QMessageBox::warning(
            this,
            tr("Cannot Save"),
            tr("The settings of %1 cannot be saved. Leave them unsaved?")
                .arg(QFileInfo{m_document->path()}.fileName()),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (answer != QMessageBox::Discard) {
            m_whenSaved = {};
            return;
        }
    }
    std::exchange(m_whenSaved, {})();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_closing || (!m_document->isModified() && !m_document->isSaving())) {
        event->accept();
        return;
    }
    // The window waits for the settings to be written, then closes again, from the event loop.
    event->ignore();
    whenSaved([this] {
        m_closing = true;
        QMetaObject::invokeMethod(this, &QWidget::close, Qt::QueuedConnection);
    });
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
    const auto image = m_document->image();
    auto *watcher = new QFutureWatcher<std::expected<WhiteBalancePick, QString>>{this};
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, image] {
        watcher->deleteLater();
        // Measured on a picture since left: not this one's white balance.
        if (m_document->image() != image) {
            return;
        }
        const auto pick = watcher->result();
        if (!pick) {
            statusBar()->showMessage(pick.error(), 6000);
            return;
        }
        // The settings may have changed meanwhile: only the white balance is the pick's.
        core::Settings settings = m_document->settings();
        for (const core::Parameter &parameter :
             {core::WhiteBalanceEffect::TEMPERATURE, core::WhiteBalanceEffect::TINT}) {
            settings.setValue(parameter.id, pick->settings.value(parameter));
        }
        m_document->setSettings(settings);
        showWhiteBalancePicked(*pick);
    });
    watcher->setFuture(QtConcurrent::run([image, settings = m_document->settings(), point, radius] {
        return pickWhiteBalance(*image, settings, point, radius);
    }));
}

void MainWindow::showWhiteBalancePicked(const WhiteBalancePick &pick)
{
    QString message = pick.clamped ? tr("White balance set as near neutral as the sliders go.")
                                   : tr("White balance set from the area picked.");
    if (pick.clipped > 0) {
        message += ' ' + tr("%n clipped pixel(s) left out.", nullptr, pick.clipped);
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

void MainWindow::chooseSettings()
{
    QSettings settings;
    const QString folder
        = settings.value(LAST_SETTINGS_FOLDER_KEY, QFileInfo{m_document->path()}.absolutePath())
              .toString();
    const QString filter = tr("Settings (%1)").arg(m_document->settingsNameFilter());
    const QString path = QFileDialog::getOpenFileName(this, tr("Load Settings"), folder, filter);
    if (!path.isEmpty()) {
        settings.setValue(LAST_SETTINGS_FOLDER_KEY, QFileInfo{path}.absolutePath());
        m_document->loadSettings(path);
    }
}
