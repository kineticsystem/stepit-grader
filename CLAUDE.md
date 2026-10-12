# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Project Overview

StepIt Grader is a C++23/Qt 6 desktop application that edits the tones and colours of one picture (TIFF, DNG, camera raw) with Lightroom-like sliders, an eyedropper for the white balance and an HDR compression, live, and saves the edit as a Lightroom-compatible XMP sidecar. It must build on Linux, macOS and Windows (MSYS2). How it is built: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md); keep it true with the `architect` skill, and review the design with the `reviewer` skill, both in `.claude/skills/`.

## Build Commands

```bash
./install-deps.sh                      # once: LibRaw and Little CMS into deps/
export Qt6_DIR=~/Qt/6.11.0/gcc_64/lib/cmake/Qt6
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build                 # or build/bin/stepit-grader-tests --gtest_filter='EffectsTest.*'
```

libtiff, pugixml and GoogleTest come from Lentigram's `lib/` (`-DLENTIGRAM_LIB`, default `../lentigram/lib`), or the system. Build in Release: the effects are about ten times slower in Debug.

## Architecture

- `src/core-lib` (namespace `core`): the engine. Standard C++ only, no Qt, no files. `Image` is linear Rec. 2020 float. Each effect is one header in `include/core/effects` and one source in `src/effects`, implementing `Effect`; `makeStandardPipeline()` is the only place that lists them.
- `src/io-lib` (namespace `imageio`): `ImageReader` (TIFF with ICC via libtiff and Little CMS, raw and DNG via the raw-converter submodule) and `SettingsStore` (`XmpSidecar`, pugixml). No Qt.
- `src/editor-lib`: Qt. Reusable widgets in `widgets/` (`ImageView`, `ParameterSlider`, `CollapsibleSection`, `AdjustmentsPanel`) that know nothing of their clients and report with signals; `Document`; `PreviewRenderer`; `MainWindow`, which only connects the parts.
- `src/editor-app`: `main()`, the composition root.
- `src/cli-lib` (namespace `cli`): `parseOptions()` and `Grader`, which grades pictures with saved settings into 16-bit TIFFs (`imageio::TiffWriter`). No Qt.
- `src/cli-app`: `stepit-grader-cli`'s `main()`.
- Dependencies go downward only: editor-app → editor-lib → io-lib → core-lib, and cli-app → cli-lib → io-lib → core-lib.
- StepIt Grader is independent software: never mention the projects that use it in its code or docs.

## Rules

- An edit is a set of named values, never pixels. A parameter's id is Lightroom's name for the setting (e.g. `Exposure2012`): never rename one, it breaks saved sidecars.
- Every effect at its defaults leaves the image unchanged, and has a test that says so and one that it moves the picture the right way.
- Inject dependencies (reader, store, pipeline) through constructors; no singletons besides the thread pool in `Parallel.cpp`.
- A widget emits signals for the user's changes only, never for a `setValue()` from its client.
- Long work runs off the main thread (`QtConcurrent`, `core::parallelFor`) and comes back with a queued call.

## Code Style

- C++23, clang-format with the `.clang-format` of the repo (Lentigram's), 100 columns.
- `m_` prefix for members, `Q_SIGNALS`/`Q_SLOTS`/`Q_EMIT`.
- Each file starts with the MIT header: `// Copyright (c) 2026 Giovanni Remigi` and `// SPDX-License-Identifier: MIT`.
- Comments say why; constants are named, at the top of their source, with what they mean.

## Testing

GoogleTest and Qt Test, in one program, `src/tests`, organised by library: `core/`, `io/`, `editor/`, `cli/`. `Main.cpp` creates one offscreen `QApplication`. Fakes of `ImageReader` and `SettingsStore` are in `testutils/Fakes.h`. The raw tests need raw-converter's samples (`modules/raw-converter/bin/fetch-samples.sh`) and are skipped without them.
