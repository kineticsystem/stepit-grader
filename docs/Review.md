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

This review judges how StepIt Grader is built, on 13 October 2026, at commit `a1693b9`. How the code works is in [ARCHITECTURE.md](ARCHITECTURE.md); this document does not repeat it.

It is a partial refresh of the review made at `1ab77b2`, brought up to date since with `LeavePictureViewModel`, `WhiteBalanceViewModel` and `PickWhiteBalanceAction`, read whole, and `MainWindow` read again. Read again, whole: `editor-lib`, about 2,800 lines, with its new view models and actions; what changed in `io-lib` since, the `CameraShotReader` interface and the XMP sidecar's names and damaged copies; and `core::match`. The parts unchanged since `1ab77b2`, `widgets-lib`, `cli-lib` and the rest of `core-lib` and `io-lib`, keep the judgements of that review.

- **Counted**: the links of each library, the includes between libraries, and the test cases of each folder.
- **Run**: the whole suite, 175 test cases in 178 runs, passing in about 10 seconds, the four raw tests skipped without raw-converter's samples; again built with AddressSanitizer and UndefinedBehaviorSanitizer, passing with no report; and with ThreadSanitizer under `setarch -R`, passing with 74 reports, none a race between two threads of our code, see [Implementation Issues](#implementation-issues).
- **Verified**: one suspicion, with a throwaway test, removed after. A finding marked *verified* was reproduced; the others are *from reading*.

## Verdict

**Sound.** The layers hold: the engine and the files know no Qt, every dependency points down, and the one idea, *an edit is a set of named values*, runs through the engine, the sidecar, the command line and the camera match alike. The editor now has the shape the last review asked for: the `Document` is the one model, a view model holds each action's rules and tells the views with signals only, and the actions hold the dialogs. Every rule the window held is now a view model's and tested without a window: saving the image, the camera match end to end with a fake reader, the white balance pick, and the wait for the settings to be written before a picture is left. The matched grade is fitted over the picture's own settings.

What needs work: the TIFF writer can still lose the file it replaces when a rename fails ([1](#1-a-save-can-harm-the-file-it-replaces)), an exception in a parallel chunk escapes while other chunks still run ([2](#2-an-exception-in-a-parallel-chunk-escapes-while-other-chunks-still-run)), and the actions, which hold the dialogs, have no test, by design.

## Structure and Boundaries

| Library | Keeps to what it owns? | Evidence |
|---|---|---|
| `core-lib` | Yes | No Qt include, no file access; `core::match` reports through `std::function` callbacks, and is given the settings it fits over in `Options::base`. |
| `io-lib` | Yes | No Qt include; Little CMS stays private in [`ColourProfiles.h`](../src/io-lib/src/ColourProfiles.h), and no public header names libtiff, LibRaw or pugixml. The sidecar asks `RawReader::canRead()` which pictures are raw, rather than keeping a list of its own. |
| `widgets-lib` | Yes | `CurveView` and `HistogramView` draw what they are given, no effect; `AdjustmentsPanel` knows parameters, not effects. |
| `editor-lib` | Yes | The view models hold the rules and the actions the dialogs; `MainWindow`, 550 lines, lays them out and connects them, see [Separation of Concerns](#separation-of-concerns). |
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
| The view models | [`viewmodel/`](../src/editor-lib/include/editor/viewmodel) | Good: `SaveImageViewModel`, `CameraLookViewModel`, `WhiteBalanceViewModel` and `LeavePictureViewModel` are given the document, the pipeline and what they read or keep with, answer calls and emit signals; no widget. Their tests drive them as the actions do. `CameraLookViewModel` keeps the last grade found after a failed match: *verified*, see [3](#3-smaller). |
| `StatusController` | [`Status.h`](../src/editor-lib/include/editor/Status.h) | Adequate: a thread-safe flag and a queued signal. One task at a time, which is all the window needs. |
| `AdjustmentsPanel` | [`AdjustmentsPanel.h`](../src/widgets-lib/include/widgets/AdjustmentsPanel.h) | Good: `addSectionWidget()`, `addSectionHeaderWidget()` and `addHeading()` let the window add to a section without the panel knowing why. `sectionTitles()` returns the titles in alphabetical order, not the order shown: a `std::map`'s, in `AdjustmentsPanel::sectionTitles()`. |

## Separation of Concerns

**`MainWindow` lays out and connects.** [`MainWindow.cpp`](../src/editor-lib/src/MainWindow.cpp), 550 lines from 713, builds the menus, the dock and the status bar, makes the actions with their view models, and shows the view models' signals. What it still decides is the window's own: the folders the open dialogs start in, the recent files and the theme, kept in `QSettings`, and which actions are enabled once a picture opens. When a picture may be left, which file may be opened and what a white balance pick changes are `LeavePictureViewModel`'s and `WhiteBalanceViewModel`'s.

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
| The sRGB curve, written out four times | Medium: a fix to one, e.g. for values below 0, misses the others. | Encoding in [`Display.cpp`](../src/core-lib/src/Display.cpp) and [`ToneCurveEffect.cpp`](../src/core-lib/src/effects/ToneCurveEffect.cpp); decoding in `ToneCurveEffect.cpp` and [`CameraLookUtils.cpp`](../src/editor-lib/src/CameraLookUtils.cpp). |
| sRGB and CIE XYZ, one matrix and its inverse typed apart | Low: both from Lindbloom's tables, but one could be edited alone. | `XYZ_TO_SRGB` in [`Color.cpp`](../src/core-lib/src/Color.cpp), `SRGB_TO_XYZ` in [`CameraMatch.cpp`](../src/core-lib/src/CameraMatch.cpp). |
| The slider ids of the camera match | Low: a test checks each names a slider of the pipeline. | String literals in `standardStages()`, against the `Parameter`s of the effects. |
| The sliders the camera match sets, told to the user | Low: a stage added to `standardStages()` would go untold. | The Apply question of [`MatchCameraLookAction::showFound()`](../src/editor-lib/src/actions/MatchCameraLookAction.cpp) names "the white balance, the tone, the tone curve and the colour mixer". |
| The highlights and the shadows | Low, and documented: the same code but for three constants, each computing the area luminance again. | [`HighlightsEffect.cpp`](../src/core-lib/src/effects/HighlightsEffect.cpp), [`ShadowsEffect.cpp`](../src/core-lib/src/effects/ShadowsEffect.cpp). |
| Two luminances | None: Rec. 2020's for the working space, Rec. 709's for the histogram of the screen's image, each where it belongs. | [`Color.h`](../src/core-lib/include/core/Color.h), [`HistogramView.cpp`](../src/widgets-lib/src/HistogramView.cpp). |

## Testability and Tests

| Folder | Test cases | Untested |
|---|---|---|
| `core` | 53 | The look of the effects, by design, see the architecture's manual checks. |
| `io` | 23 | `TiffWriter` replacing a file when the rename fails, finding [1](#1-a-save-can-harm-the-file-it-replaces). |
| `editor` | 92 | `MainWindow` and the actions: no test constructs them. |
| `cli` | 7 | |

**No seam is missing in the editor.** Each rule the window held has its view model's tests: 7 for `SaveImageViewModel`, 5 for `CameraLookViewModel`, the whole match on a fake reader in about 20 ms each, 9 for `LeavePictureViewModel`, with a failing save from `FakeStore`'s `failSaving`, and 3 for `WhiteBalanceViewModel`. One rule is not tested: a pick measured on a picture since left is dropped, as no test can time a pick against an open without a hook in the view model.

The actions are not tested, by design: they hold the dialogs, which a test cannot answer.

## Implementation Issues

Ordered by severity. ThreadSanitizer's 74 reports are all a value handed to a pooled thread through Qt's thread pool, whose synchronisation an uninstrumented Qt hides: the copies `QtConcurrent::run` makes of a lambda's captures on the main thread, read on the pooled thread, in [`Document.cpp`](../src/editor-lib/src/Document.cpp) at the open and the load of settings, and a test's slot connected without a context, run on the match's thread. None is in a view model or an action, nor in `core::parallelFor`, the renderer's generation counter or `core::match`.

### 1. A save can harm the file it replaces

*From reading.* [`TiffWriter::write()`](../src/io-lib/src/TiffWriter.cpp) calls `std::filesystem::remove(path)` and then `rename()`, on every system, for Windows' sake. If the rename then fails, the code also removes the temporary file: the old TIFF and the new one are both gone, against the architecture's claim that a failure never harms the file it would replace. [`XmpSidecar::save()`](../src/io-lib/src/XmpSidecar.cpp) renames without removing; one of the two is wrong on Windows.

Two more ways, through **Save Image…**: the temporary file is named after the file alone, `.<name>.part`, so two saves to the same file at once, which `SaveImageViewModel::save()` allows, write the same temporary file; and [`ImageExportUtils::saveImage()`](../src/editor-lib/src/ImageExportUtils.cpp) writes a JPEG with `QImageWriter` straight over the file, so a failed write leaves part of it.

### 2. An exception in a parallel chunk escapes while other chunks still run

*Verified at `1ab77b2`; the code is unchanged.* A chunk of [`core::parallelFor()`](../src/core-lib/src/Parallel.cpp) that throws on the calling thread leaves `ThreadPool::run()` at once. The chunks still running on other threads then count down a `Group` that lived on the unwound stack. A chunk that throws on a worker ends the program. Only `std::bad_alloc` can throw there today, on a picture too large for the memory.

### 3. Smaller

- `CameraLookViewModel` keeps the last grade found after a later match failed: `summary()` returns its text and `apply()` merges it, *verified*. The action calls `apply()` only after `finished()`, so the user cannot reach it today; a second view could. [`CameraLookViewModel::start()`](../src/editor-lib/src/viewmodel/CameraLookViewModel.cpp) does not clear `m_result`.
- `ParameterSlider::show(double)`, private, hides `QWidget::show()`: `slider.show()` does not compile; [`ParameterSlider.h`](../src/widgets-lib/include/widgets/ParameterSlider.h).
- **Match Camera Look…** on an open raw file reads it again from disk, about 3 seconds, though the document holds it: `CameraLookViewModel` passes the path, not the image.
- `XmpSidecar::save()` keeps a damaged sidecar as `.damaged`, overwriting the copy of an earlier damage.

## Recommendations

Ordered by value against effort; each is one pull request.

| # | Change | Fixes | Trade-off |
|---|---|---|---|
| 1 | Rename over the old TIFF without removing it first, and remove it only where the rename fails because it exists; name the temporary file uniquely; write a JPEG to a temporary file too. | 1 | One more branch, tested on Windows. |
| 2 | Catch an exception in each chunk of `parallelFor`, keep the first, wait for every chunk, then throw it. | 2 | A few lines in the one place every effect goes through. |
| 3 | One sRGB curve, each way, and one sRGB-to-XYZ matrix, in `core::color`. | Duplication | None. |
| 4 | Clear `CameraLookViewModel`'s result when a match starts, and build the Apply question's list of sliders from the stages. | 3, coupling | None. |
| 5 | Keep the sections' order in `AdjustmentsPanel`, and rename `ParameterSlider::show()` to `showValue()`. | 3 and the panel's titles | None. |
