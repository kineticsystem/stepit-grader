# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Project Overview

StepIt Grader is a C++23/Qt 6 desktop application that edits the tones and colours of one picture (TIFF, DNG, camera raw) with the usual sliders of a raw editor, an eyedropper for the white balance and an HDR compression, live, and saves the edit by itself, a second after the last change, as an XMP sidecar next to the picture. It must build on Linux, macOS and Windows (MSYS2). How it is built: [docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md); keep it true with the `architect` skill, and review the design with the `reviewer` skill, which keeps [docs/Review.md](../docs/Review.md), both in `.claude/skills/`.

## Build Commands

```bash
./install-deps.sh                      # once: LibRaw, Little CMS, libtiff, pugixml, GoogleTest into deps/
export Qt6_DIR=~/Qt/6.11.0/gcc_64/lib/cmake/Qt6
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build                 # or build/bin/stepit-grader-tests --gtest_filter='EffectsTest.*'
```

LibRaw, Little CMS, libtiff, pugixml and GoogleTest come from `deps/<library>`, built by `./install-deps.sh` (one script per library in `scripts/`) and found by the modules of `cmake/`; without `deps/`, from the system. Build in Release; `core-lib` is optimised in Debug too, its effects being about ten times slower without the optimiser. With `-DSTEPIT_GRADER_ENGINE_ONLY=ON` (and `STEPIT_GRADER_BUILD_WIDGETS`) the project builds only `core-lib` and `widgets-lib`, without `deps/`, for another program that adds it with `add_subdirectory()` and links `stepit-grader::core` and `stepit-grader::widgets`.

## Packaging

`setup/linux/package-stepit-grader.sh` makes the Linux AppImage, with both programs (`--cli` for the command line), into `build/linux-package/`. It needs `./install-deps.sh` first, which downloads linuxdeploy into `deps/linuxdeploy`.

## Architecture

- `src/core-lib` (namespace `core`): the engine. Standard C++ only, no Qt, no files. `Image` is linear Rec. 2020 float. Each effect is one header in `include/core/effects` and one source in `src/effects`, implementing `Effect`; `makeStandardPipeline()` is the only place that lists them. `core::match` fits the sliders to a camera's JPEGs.
- `src/io-lib` (namespace `imageio`): `ImageReader` (TIFF with ICC via libtiff and Little CMS, raw and DNG via the raw-converter submodule) and `SettingsStore` (`XmpSidecar`, pugixml). No Qt.
- `src/widgets-lib`: Qt Widgets and `core-lib` only. The reusable widgets, included as `widgets/...` (`ImageView`, `ParameterSlider`, `CollapsibleSection`, `AdjustmentsPanel`, `CurveView`, `HistogramView`, `SectionUtils::sectionsOf()`, `SliderStyle`, whose `apply()` gives any slider the panel's look), that know nothing of their clients and report with signals. Only what another program needs to grade: a widget of the editor's own, e.g. `RecentFilesMenu`, is in `editor-lib`.
- `src/editor-lib`: Qt. `Document`, which reads and writes every file in turn on a one-thread pool and saves the settings by itself after a delay it is given; `PreviewRenderer`; `RecentFilesMenu` and `ThemeMenu`; `SessionProperties`, the keys of everything kept in `QSettings`; `FileUtils`, the open dialogs that remember their folder; `ConcurrentUtils`, background work whose result comes back to the main thread; `CameraLookUtils::matchCameraLook()`, which runs `core::match::fit()` on raw files and their camera JPEGs read by an `imageio::CameraShotReader`, over the picture's settings, reporting to a `StatusController` that a `WaitingDialog` follows, from Lentigram's status classes; the view models of `viewmodel/`, `SaveImageViewModel`, `CameraLookViewModel`, `WhiteBalanceViewModel` and `LeavePictureViewModel`, which writes the settings before a picture is left, which hold the rules of an action and tell the views with signals only, and the actions of `actions/`, the views, given their view model, which hold only the dialogs; `MainWindow`, which only connects the parts.
- `src/editor-app`: `main()`, the composition root.
- `src/cli-lib` (namespace `cli`): `parseOptions()` and `Grader`, which grades pictures with saved settings into 16-bit TIFFs (`imageio::TiffWriter`). No Qt.
- `src/cli-app`: `stepit-grader-cli`'s `main()`.
- Dependencies go downward only: editor-app → editor-lib → widgets-lib, io-lib → core-lib, and cli-app → cli-lib → io-lib → core-lib.
- StepIt Grader is independent software: never mention the projects that use it in its code or docs.

## Rules

- An edit is a set of named values, never pixels. A parameter's id is its name in the sidecar's XMP namespace (e.g. `Exposure2012`), the name other raw editors use too: never rename one, it breaks saved sidecars.
- Every effect at its defaults leaves the image unchanged, and has a test that says so and one that it moves the picture the right way.
- Inject dependencies (reader, store, pipeline) through constructors; no singletons besides the thread pool in `Parallel.cpp`.
- A widget emits signals for the user's changes only, never for a `setValue()` from its client.
- Long work runs off the main thread (`ConcurrentUtils::run()` in a view model, `QtConcurrent`, `core::parallelFor`) and comes back with a queued call. So does every menu action's work, a sidecar's read and write included: the window waits for a result through signals, never by blocking.

## Code Style

- C++23, clang-format with the `.clang-format` of the repo, 100 columns.
- `m_` prefix for members, `Q_SIGNALS`/`Q_SLOTS`/`Q_EMIT`.
- Each file starts with the MIT header: `// Copyright (c) 2026 Giovanni Remigi` and `// SPDX-License-Identifier: MIT`.
- Comments say why; constants are named, at the top of their source, with what they mean.
- Stateless free functions that belong together go in a utility namespace named after their topic, in a file of the same name, e.g. `namespace ThemeUtils` in `ThemeUtils.h`, never at global scope, with a comment before the namespace that says what its functions are for and who uses them, as a class has. `core-lib`, `io-lib` and `cli-lib` keep theirs in their library's namespace, grouped by topic, e.g. `core::color`.

## Testing

GoogleTest and Qt Test, in one program, `src/tests`, organised by library: `core/`, `io/`, `editor/`, `cli/`. `Main.cpp` creates one offscreen `QApplication`. Fakes of `ImageReader` and `SettingsStore` are in `testutils/Fakes.h`. The raw tests need raw-converter's samples (`modules/raw-converter/bin/fetch-samples.sh`) and are skipped without them.
