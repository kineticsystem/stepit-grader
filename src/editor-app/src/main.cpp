// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/Document.h"
#include "editor/MainWindow.h"
#include "editor/PreviewRenderer.h"
#include "editor/Sections.h"
#include "editor/Theme.h"

#include "core/StandardPipeline.h"
#include "imageio/ImageLoader.h"
#include "imageio/XmpSidecar.h"

#include <QApplication>
#include <QIcon>
#include <QCommandLineParser>
#include <QSettings>

#ifdef __GLIBC__
#include <malloc.h>
#endif

/// The composition root: the one place that decides which parts make the editor.
int main(int argc, char *argv[])
{
#ifdef __GLIBC__
    // Every effect allocates planes of tens of MB. By default glibc gives each back to the kernel
    // when freed, and the next one comes back zeroed, page by page: half the time of a preview.
    // Kept instead, they are reused.
    mallopt(M_MMAP_THRESHOLD, 1 << 30);
    mallopt(M_TRIM_THRESHOLD, 1 << 30);
#endif
    QApplication app{argc, argv};
    QApplication::setOrganizationName("kineticsystem");
    QApplication::setApplicationName("StepIt Grader");
    QApplication::setApplicationVersion(STEPIT_GRADER_VERSION);
    QApplication::setWindowIcon(QIcon{":/stepit-grader.png"});
    // The desktop file's name, so the desktop matches the window to its icon.
    QApplication::setDesktopFileName("stepit-grader");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QApplication::translate("main", "Edits the tones and colours of a picture."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(
        "image", QApplication::translate("main", "The TIFF, DNG or raw file to open."), "[image]");
    parser.process(app);

    // The theme chosen in Settings > Theme, before the window and its icons are made.
    applyTheme(themeNamed(QSettings{}.value(THEME_SETTING).toString()).value_or(Theme::System));
#ifdef Q_OS_MACOS
    // macOS's menu bar stays light whatever the theme, where the icons of a dark
    // theme would hardly show. The menus go without icons; the toolbar keeps them.
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    const auto pipeline = core::makeStandardPipeline();
    Document document{
        imageio::makeStandardImageReader(),
        std::make_shared<imageio::XmpSidecar>(),
        pipeline->parameters()};
    PreviewRenderer renderer{pipeline};
    MainWindow window{&document, &renderer, sectionsOf(*pipeline)};
    window.show();

    if (!parser.positionalArguments().isEmpty()) {
        window.openFile(parser.positionalArguments().front());
    }
    return app.exec();
}
