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

This review judges how StepIt Grader is built, on 13 October 2026, at the commit that holds this document. How the code works is in [ARCHITECTURE.md](ARCHITECTURE.md); this document does not repeat it.

- **Read**, whole: `editor-lib`, about 3,700 lines, its view models, actions, mediators and utilities; in `io-lib`, the readers' interfaces, `imageio::files` and the XMP sidecar; in `core-lib`, `core::match`, `core::color` and `core::parallelFor`; the tests of each. `widgets-lib`, `cli-lib` and the other parts of `core-lib` and `io-lib` are judged on their code at `1ab77b2`, which is the code they hold.
- **Counted**: the links of each library, the includes between libraries, and the test cases of each folder.
- **Run**: the whole suite, 199 test cases in 202 runs, passing in about 10 seconds, the four raw tests skipped without raw-converter's samples; again built with AddressSanitizer and UndefinedBehaviorSanitizer, passing with no report; and with ThreadSanitizer under `setarch -R`, passing with 78 reports, none a race between two threads of our code, see [Implementation Issues](#implementation-issues).
- Every finding is *from reading*: none needed a probe to be told apart from a suspicion.

## Verdict

**Sound.** The layers hold: the engine and the files know no Qt, every dependency points down, and the one idea, *an edit is a set of named values*, runs through the engine, the sidecar, the command line and the camera match alike. In the editor, the `Document` is the one model, a view model holds each action's rules and tells the views with signals only, the actions hold the dialogs, and mediators keep the parts in step. Each rule is tested without a window: saving the image, the camera match end to end with a fake reader, the white balance pick, and the wait for the settings to be written before a picture is left. A save never harms the file it replaces, an exception in a parallel chunk reaches its caller, and the sRGB curve and matrix are written once.

What needs work: small things, [1](#1-smaller), and the actions, which hold the dialogs, have no test, by design.

## Structure and Boundaries

| Library | Keeps to what it owns? | Evidence |
|---|---|---|
| `core-lib` | Yes | No Qt include, no file access; `core::match` reports through `std::function` callbacks, and is given the settings it fits over in `Options::base`. |
| `io-lib` | Yes | No Qt include; Little CMS stays private in [`ColourProfiles.h`](../src/io-lib/src/ColourProfiles.h), and no public header names libtiff, LibRaw or pugixml. The sidecar asks `RawReader::canRead()` which pictures are raw, rather than keeping a list of its own. |
| `widgets-lib` | Yes | `CurveView` and `HistogramView` draw what they are given, no effect; `AdjustmentsPanel` knows parameters, not effects. |
| `editor-lib` | Yes | The view models hold the rules, the actions the dialogs, the mediators the connections; `MainWindow`, 350 lines, lays them out and makes them, see [Separation of Concerns](#separation-of-concerns). |
| `cli-lib` | Yes | `Grader` returns results; `main()` prints them. |

## Dependency Direction

Only downward, as `CLAUDE.md` asks: `editor-app` → `editor-lib` → `widgets-lib`, `io-lib` → `core-lib`, and `cli-app` → `cli-lib` → `io-lib` → `core-lib`.

| Library | Links | Includes from other libraries | Against the rule |
|---|---|---|---|
| `core-lib` | Threads | none | none |
| `io-lib` | `core-lib`; privately raw-converter, libtiff, pugixml, Little CMS | 10 of `core/` | none |
| `widgets-lib` | `core-lib`, Qt Widgets | 3 of `core/` | none |
| `editor-lib` | `core-lib`, `widgets-lib`, `io-lib`, Qt Widgets, Concurrent, Svg | 27 of `core/`, 9 of `imageio/`, 5 of `widgets/` | none |
| `cli-lib` | `core-lib`, `io-lib` | 1 of `core/`, 4 of `imageio/` | none |
| `editor-app`, `cli-app` | their library | `core/`, `imageio/`, `widgets/`, to compose | none: the composition roots |

No view model includes a widget; of the actions, the views, only `PickWhiteBalanceAction` does, the `ImageView` it puts in pick mode. `MainWindow` includes nothing of `imageio/`.

## Interfaces and Abstractions

| Interface | Where | Judgement |
|---|---|---|
| `Effect` | [`Effect.h`](../src/core-lib/include/core/Effect.h) | Good: 17 effects, each a file; adding one is a file and a line in `makeStandardPipeline()`. `isIdentity()` lets the pipeline skip what changes nothing. |
| `Pipeline::render()` | [`Pipeline.h`](../src/core-lib/include/core/Pipeline.h) | Good: a `cancelled` callback, so the renderer and the camera match stop early without the engine knowing them. |
| `ImageReader` | [`ImageReader.h`](../src/io-lib/include/imageio/ImageReader.h) | Good: three implementations and a fake. |
| `CameraShotReader` | [`CameraShotReader.h`](../src/io-lib/include/imageio/CameraShotReader.h) | Good: one implementation, `RawShotReader`, which defers to `RawReader` and `readCameraShot()`, and a fake, so `CameraLookUtils::matchCameraLook()` is tested without raw files. |
| `SettingsStore` | [`SettingsStore.h`](../src/io-lib/include/imageio/SettingsStore.h) | Good: one implementation, the XMP sidecar, and a fake; where a sidecar lives is the store's to say, `location()`, so the document and the command line never build its name. |
| `core::match::Options` | [`CameraMatch.h`](../src/core-lib/include/core/CameraMatch.h) | Good: progress and stop as plain callbacks, and `base`, the settings the grade goes over, so the search runs, and is tested, without Qt. |
| The view models | [`viewmodel/`](../src/editor-lib/include/editor/viewmodel) | Good: `SaveImageViewModel`, `CameraLookViewModel`, `WhiteBalanceViewModel` and `LeavePictureViewModel` are given the document, the pipeline and what they read or keep with, answer calls and emit signals; no widget. Their tests drive them as the actions do. |
| `StatusController` | [`Status.h`](../src/editor-lib/include/editor/Status.h) | Adequate: a thread-safe flag and a queued signal. One task at a time, which is all the window needs. |
| `AdjustmentsPanel` | [`AdjustmentsPanel.h`](../src/widgets-lib/include/widgets/AdjustmentsPanel.h) | Good: `addSectionWidget()`, `addSectionHeaderWidget()` and `addHeading()` let the window add to a section without the panel knowing why. `sectionTitles()` gives the titles in the order shown. |

## Separation of Concerns

**`MainWindow` lays out and connects.** [`MainWindow.cpp`](../src/editor-lib/src/MainWindow.cpp), 350 lines, builds the menus, the toolbar and the dock, and makes the actions with their view models and the four mediators that keep the parts in step: the document and the panel, the renderer and the views, the status bar, and the dialogs. What it decides is the window's own: the folders the open dialogs start in, the recent files and the theme, kept in `QSettings`, and which actions are enabled once a picture opens. When a picture may be left, which file may be opened and what a white balance pick changes are `LeavePictureViewModel`'s and `WhiteBalanceViewModel`'s.

Elsewhere, knowledge sits where it belongs: the document owns the files and their order, the view models own the rules of the actions, the renderer owns the passes, the engine owns the pixels and the search.

## SOLID

| Principle | Verdict | Where |
|---|---|---|
| Single responsibility | No violation found | `MainWindow` lays out and connects, above. The effects, `Document`, `PreviewRenderer`, `Grader` and each view model have one. |
| Open/closed | No violation found | An effect, a reader or an output format is added beside the others: `makeStandardPipeline()`, `makeStandardImageReader()`, `ImageExportUtils::imageFormats()`; an action, as a view model and an action beside the two there are. |
| Liskov substitution | No violation found | `FakeReader`, `FakeStore` and `FakeShotReader` stand in for the real ones in the document, view model and command line tests. |
| Interface segregation | No violation found | `SettingsStore` has five functions, all used by the document; the command line uses two, `load()` and `loadFile()`. `CameraShotReader` has the three the camera match uses. |
| Dependency inversion | One small violation | `SaveImageViewModel::save()` calls `ImageExportUtils::saveImage()`, which writes with a concrete `imageio::TiffWriter` or `QImageWriter`, so its tests write real files. `Document`, `PreviewRenderer`, `CameraLookViewModel` and `Grader` are given their parts, and `main()` gives `MainWindow` its view models. |

## Coupling and Duplication

| Kept in step by hand | Weight | Where |
|---|---|---|
| The slider ids of the camera match | Low: a test checks each names a slider of the pipeline. | String literals in `standardStages()`, against the `Parameter`s of the effects. |
| The highlights and the shadows | Low, and documented: the same code but for three constants, each computing the area luminance again. | [`HighlightsEffect.cpp`](../src/core-lib/src/effects/HighlightsEffect.cpp), [`ShadowsEffect.cpp`](../src/core-lib/src/effects/ShadowsEffect.cpp). |
| Two luminances | None: Rec. 2020's for the working space, Rec. 709's for the histogram of the screen's image, each where it belongs. | [`Color.h`](../src/core-lib/include/core/Color.h), [`HistogramView.cpp`](../src/widgets-lib/src/HistogramView.cpp). |

## Testability and Tests

| Folder | Test cases | Untested |
|---|---|---|
| `core` | 58 | The look of the effects, by design, see the architecture's manual checks. |
| `io` | 27 | `imageio::files::replace()` setting the old file aside where a system does not rename over a file: Linux always does. |
| `editor` | 107 | `MainWindow` and the actions: no test constructs them. |
| `cli` | 7 | |

**No seam is missing in the editor.** Each rule of the actions has its view model's tests: 7 for `SaveImageViewModel`, 7 for `CameraLookViewModel`, the whole match on a fake reader in about 20 ms each, 9 for `LeavePictureViewModel`, with a failing save from `FakeStore`'s `failSaving`, and 3 for `WhiteBalanceViewModel`. One rule is not tested: a pick measured on a picture since left is dropped, as no test can time a pick against an open without a hook in the view model.

The actions are not tested, by design: they hold the dialogs, which a test cannot answer.

## Implementation Issues

Ordered by severity. ThreadSanitizer's 78 reports are all a value handed to a pooled thread through Qt's thread pool, whose synchronisation an uninstrumented Qt hides: the copies `QtConcurrent::run` makes of a lambda's captures on the main thread, read on the pooled thread, in [`Document.cpp`](../src/editor-lib/src/Document.cpp) at the open and the load of settings, and a test's slot connected without a context, run on the match's thread. None is in a view model or an action, nor in `core::parallelFor`, `ConcurrentUtils`, the renderer's generation counter or `core::match`.

### 1. Smaller

- **Match Camera Look…** on an open raw file reads it again from disk, about 3 seconds, though the document holds it: `CameraLookViewModel` passes the path, not the image.
- `XmpSidecar::save()` keeps a damaged sidecar as `.damaged`, overwriting the copy of an earlier damage.

## Recommendations

Ordered by value against effort; each is one pull request.

| # | Change | Fixes | Trade-off |
|---|---|---|---|
| 1 | Give the camera match the document's picture when the picture open is the raw file matched, and read only its camera JPEG from the file. | 1, the raw file read again | `CameraShotReader` reads the JPEG alone too: one more function to fake. |
| 2 | Name each damaged copy of a sidecar apart, e.g. `IMG_0001.xmp.damaged.2`, or keep the first. | 1, the damaged copy overwritten | A folder may gather copies; the first is the one most worth keeping. |
