# StepIt Grader Review

## Table of Contents <!-- omit in toc -->

- [Scope and Method](#scope-and-method)
- [Verdict](#verdict)
- [Structure and Boundaries](#structure-and-boundaries)
- [Dependency Direction](#dependency-direction)
- [Interfaces and Abstractions](#interfaces-and-abstractions)
- [Separation of Concerns](#separation-of-concerns)
- [SOLID](#solid)
- [Coupling and Duplication](#coupling-and-duplication)
- [Testability and Tests](#testability-and-tests)
- [Implementation Issues](#implementation-issues)
- [Recommendations](#recommendations)

## Scope and Method

This review judges how StepIt Grader is built, as of 13 October 2026, at the commit that contains this document. [ARCHITECTURE.md](ARCHITECTURE.md) explains how the code works; this document does not repeat it.

- **Read in full**: `editor-lib` (about 3,700 lines: its view models, actions, mediators and utilities); in `io-lib`, the reader interfaces, `imageio::files` and the XMP sidecar; in `core-lib`, `core::match`, `core::color` and `core::parallelFor`; and the tests of each. `widgets-lib`, `cli-lib` and the rest of `core-lib` and `io-lib` are judged on their code at `1ab77b2`, which is their current code.
- **Counted**: the links of each library, the includes between libraries, and the test cases in each folder.
- **Run**: the whole suite, 199 test cases in 202 runs, in about 10 seconds, with the four raw tests skipped for lack of samples. Also built with AddressSanitizer and UndefinedBehaviorSanitizer: no reports. And with ThreadSanitizer under `setarch -R`: 78 reports, none of them a race between two of our own threads; see [Implementation Issues](#implementation-issues).
- Every finding below comes from reading the code. None needed a test to confirm it.

## Verdict

**Sound.** The layers hold. The engine and the file code use no Qt, and every dependency points downward. The core idea, *an edit is a set of named values*, runs through the engine, the sidecar, the command-line tool and the camera match alike.

In the editor, the `Document` is the single model. Each action's rules live in a view model, which reports to the views with signals only. The actions hold the dialogs, and mediators keep the parts in step. Every rule is tested without a window: saving the image, the camera match end to end with a fake reader, the white balance pick, and waiting for the settings to be written before leaving a picture. A save never harms the file it replaces. An exception in a parallel chunk reaches its caller. The sRGB curve and matrix are written once.

What needs work: two small issues ([1](#1-smaller)). The actions, which hold the dialogs, have no tests, by design.

## Structure and Boundaries

| Library | Keeps to its role? | Evidence |
|---|---|---|
| `core-lib` | Yes | No Qt includes and no file access. `core::match` reports through `std::function` callbacks, and receives the settings to fit over in `Options::base`. |
| `io-lib` | Yes | No Qt includes. Little CMS stays private in [`ColourProfiles.h`](../src/io-lib/src/ColourProfiles.h), and no public header names libtiff, LibRaw or pugixml. The sidecar asks `RawReader::canRead()` which files are raw, instead of keeping its own list. |
| `widgets-lib` | Yes | `CurveView` and `HistogramView` draw what they are given and know no effect. `AdjustmentsPanel` knows parameters, not effects. |
| `editor-lib` | Yes | View models hold the rules, actions hold the dialogs, mediators hold the connections. `MainWindow`, 350 lines, lays them out and creates them; see [Separation of Concerns](#separation-of-concerns). |
| `cli-lib` | Yes | `Grader` returns results, and `main()` prints them. |

## Dependency Direction

Dependencies only point downward, as `CLAUDE.md` requires: `editor-app` → `editor-lib` → `widgets-lib`, `io-lib` → `core-lib`, and `cli-app` → `cli-lib` → `io-lib` → `core-lib`.

| Library | Links | Includes from other libraries | Against the rule |
|---|---|---|---|
| `core-lib` | Threads | None | None |
| `io-lib` | `core-lib`; privately raw-converter, libtiff, pugixml, Little CMS | 10 from `core/` | None |
| `widgets-lib` | `core-lib`, Qt Widgets | 3 from `core/` | None |
| `editor-lib` | `core-lib`, `widgets-lib`, `io-lib`, Qt Widgets, Concurrent, Svg | 27 from `core/`, 9 from `imageio/`, 5 from `widgets/` | None |
| `cli-lib` | `core-lib`, `io-lib` | 1 from `core/`, 4 from `imageio/` | None |
| `editor-app`, `cli-app` | Their library | `core/`, `imageio/`, `widgets/`, to put the parts together | None: these are the composition roots |

No view model includes a widget. Among the actions, only `PickWhiteBalanceAction` includes one: the `ImageView` it puts in pick mode. `MainWindow` includes nothing from `imageio/`.

## Interfaces and Abstractions

| Interface | Where | Judgement |
|---|---|---|
| `Effect` | [`Effect.h`](../src/core-lib/include/core/Effect.h) | Good. 17 effects, one file each; adding one takes a file and a line in `makeStandardPipeline()`. `isIdentity()` lets the pipeline skip effects that change nothing. |
| `Pipeline::render()` | [`Pipeline.h`](../src/core-lib/include/core/Pipeline.h) | Good. A `cancelled` callback lets the renderer and the camera match stop early without the engine knowing about them. |
| `ImageReader` | [`ImageReader.h`](../src/io-lib/include/imageio/ImageReader.h) | Good. Three implementations and a fake. |
| `CameraShotReader` | [`CameraShotReader.h`](../src/io-lib/include/imageio/CameraShotReader.h) | Good. One implementation, `RawShotReader`, which delegates to `RawReader` and `readCameraShot()`, and a fake, so `CameraLookUtils::matchCameraLook()` is tested without raw files. |
| `SettingsStore` | [`SettingsStore.h`](../src/io-lib/include/imageio/SettingsStore.h) | Good. One implementation, the XMP sidecar, and a fake. The store decides where a sidecar lives (`location()`), so the document and the command-line tool never build its name. |
| `core::match::Options` | [`CameraMatch.h`](../src/core-lib/include/core/CameraMatch.h) | Good. Progress and stopping are plain callbacks, and `base` holds the settings the grade goes over. So the search runs, and is tested, without Qt. |
| The view models | [`viewmodel/`](../src/editor-lib/include/editor/viewmodel) | Good. `SaveImageViewModel`, `CameraLookViewModel`, `WhiteBalanceViewModel` and `LeavePictureViewModel` are given the document, the pipeline and what they read or store with. They answer calls and emit signals, and have no widgets. Their tests drive them the way the actions do. |
| `ProgressMonitor` | [`ProgressMonitor.h`](../src/editor-lib/include/editor/ProgressMonitor.h) | Adequate. A thread-safe flag and a queued signal. It handles one task at a time, which is all the window needs. |
| `AdjustmentsPanel` | [`AdjustmentsPanel.h`](../src/widgets-lib/include/widgets/AdjustmentsPanel.h) | Good. `addSectionWidget()`, `addSectionHeaderWidget()` and `addHeading()` let the window add to a section without the panel knowing why. `sectionTitles()` returns the titles in display order. |

## Separation of Concerns

**`MainWindow` lays out and connects.** [`MainWindow.cpp`](../src/editor-lib/src/MainWindow.cpp) has 350 lines. It builds the menus, the toolbar and the dock, and creates the actions with their view models. It also creates the four mediators that keep the parts in step: document and panel, renderer and views, status bar, and dialogs.

The only things it decides belong to the window itself: the dialogs' start folders, the recent files and the theme (all stored in `QSettings`), and which actions are enabled once a picture is open. When a picture may be left, which file may be opened, and what a white balance pick changes are decided by `LeavePictureViewModel` and `WhiteBalanceViewModel`.

Elsewhere, each piece of knowledge is where it belongs. The document owns the files and their order. The view models own the actions' rules. The renderer owns the passes. The engine owns the pixels and the search.

## SOLID

| Principle | Verdict | Where |
|---|---|---|
| Single responsibility | No violation found | `MainWindow` lays out and connects; see above. The effects, `Document`, `PreviewRenderer`, `Grader` and each view model each have one job. |
| Open/closed | No violation found | A new effect, reader or output format is added next to the existing ones: `makeStandardPipeline()`, `makeStandardImageReader()`, `ImageExportUtils::imageFormats()`. A new action is a view model and an action, next to the existing ones. |
| Liskov substitution | No violation found | `FakeReader`, `FakeStore` and `FakeShotReader` replace the real ones in the document, view model and command-line tests. |
| Interface segregation | No violation found | `SettingsStore` has five functions, all used by the document; the command-line tool uses two, `load()` and `loadFile()`. `CameraShotReader` has the three functions the camera match uses. |
| Dependency inversion | One small violation | `SaveImageViewModel::save()` calls `ImageExportUtils::saveImage()`, which writes with a concrete `imageio::TiffWriter` or `QImageWriter`. So its tests write real files. `Document`, `PreviewRenderer`, `CameraLookViewModel` and `Grader` are given their parts, and `main()` gives `MainWindow` its view models. |

## Coupling and Duplication

| Kept in step by hand | Weight | Where |
|---|---|---|
| The slider ids of the camera match | Low: a test checks that each one names a slider of the pipeline. | String literals in `standardStages()`, matched against the effects' `Parameter`s. |
| The highlights and the shadows | Low, and documented: the same code except for three constants. Each computes the area luminance again. | [`HighlightsEffect.cpp`](../src/core-lib/src/effects/HighlightsEffect.cpp), [`ShadowsEffect.cpp`](../src/core-lib/src/effects/ShadowsEffect.cpp). |
| Two luminances | None: Rec. 2020's for the working space and Rec. 709's for the histogram of the on-screen image. Each is in the right place. | [`Color.h`](../src/core-lib/include/core/Color.h), [`HistogramView.cpp`](../src/widgets-lib/src/HistogramView.cpp). |

## Testability and Tests

| Folder | Test cases | Untested |
|---|---|---|
| `core` | 58 | The look of the effects, by design; see the manual checks in the architecture. |
| `io` | 27 | The fallback in `imageio::files::replace()` that sets the old file aside on systems that do not rename over a file. Linux always does, so it cannot be triggered here. |
| `editor` | 107 | `MainWindow` and the actions: no test creates them. |
| `cli` | 7 | |

**No seam is missing in the editor.** Each action's rules are tested through its view model: 7 tests for `SaveImageViewModel`; 7 for `CameraLookViewModel`, running the whole match on a fake reader in about 20 ms each; 9 for `LeavePictureViewModel`, using `FakeStore`'s `failSaving` for failed saves; and 3 for `WhiteBalanceViewModel`. One rule is untested: a pick measured on a picture that has since been left is dropped. A test cannot time a pick against an open without a hook in the view model.

The actions are not tested, by design. They hold the dialogs, which a test cannot answer.

## Implementation Issues

The issues are ordered by severity.

All 78 ThreadSanitizer reports involve a value handed to a pooled thread through Qt's thread pool, whose synchronisation is hidden because Qt is not instrumented. There are two kinds:

- `QtConcurrent::run` copies a lambda's captures on the main thread, and a pooled thread reads them. This happens in [`Document.cpp`](../src/editor-lib/src/Document.cpp), when opening a picture and loading settings.
- A test connects a slot without a context object, so the slot runs on the match's thread.

None of the reports is in a view model, an action, `core::parallelFor`, `ConcurrentUtils`, the renderer's generation counter or `core::match`.

### 1. Smaller

- **Match Camera Look…** on an open raw file reads the file from disk again (about 3 seconds), even though the document already holds the picture. `CameraLookViewModel` passes the path, not the image.
- `XmpSidecar::save()` keeps a damaged sidecar as `.damaged`. If the sidecar is damaged a second time, the new copy overwrites the first.

## Recommendations

Ordered by value for the effort. Each fits in one pull request.

| # | Change | Fixes | Trade-off |
|---|---|---|---|
| 1 | When the open picture is the raw file being matched, give the camera match the document's picture, and read only the camera JPEG from the file. | 1, the raw file read again | `CameraShotReader` needs a function that reads only the JPEG: one more function to fake. |
| 2 | Give each damaged copy of a sidecar its own name, such as `IMG_0001.xmp.damaged.2`, or keep only the first. | 1, the damaged copy overwritten | A folder may collect several copies; the first is usually the most useful. |
