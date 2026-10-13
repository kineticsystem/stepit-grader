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

This review judges how StepIt Grader is built, on 13 October 2026, at commit `1ab77b2`. How the code works is in [ARCHITECTURE.md](ARCHITECTURE.md); this document does not repeat it.

- **Read**: every source file of every library and program, about 9,400 lines, the headers of the parts judged, each `CMakeLists.txt`, and the tests, about 3,300 lines.
- **Counted**: the links of each library, the includes between libraries, the copies of the same code, and the test cases of each folder.
- **Run**: the whole suite, 149 test cases in 152 runs, passing in about 20 seconds; again built with AddressSanitizer and UndefinedBehaviorSanitizer, passing with no report; and with ThreadSanitizer under `setarch -R`, passing with 85 reports, none with both accesses in our code, see [Implementation Issues](#implementation-issues).
- **Verified**: two suspicions, each with a throwaway test, removed after. A finding marked *verified* was reproduced; the others are *from reading*.

## Verdict

**Sound.** The layers hold: the engine and the files know no Qt, every dependency points down, and the one idea, *an edit is a set of named values*, runs through the engine, the sidecar, the command line and the camera match alike. The effects, the readers and the store sit behind small interfaces that the tests fake, and the sanitizers find nothing in our code.

What needs work: the TIFF writer can lose the file it replaces when a rename fails ([1](#1-the-tiff-writer-deletes-the-file-it-replaces-before-replacing-it)), an exception in a parallel chunk escapes while other chunks still run ([2](#2-an-exception-in-a-parallel-chunk-escapes-while-other-chunks-still-run)), and `MainWindow`, at 713 lines, has started to hold policy that belongs elsewhere, untested because nothing constructs it.

## Structure and Boundaries

| Library | Keeps to what it owns? | Evidence |
|---|---|---|
| `core-lib` | Yes | No Qt include, no file access; `core::match` reports through `std::function` callbacks, not signals. |
| `io-lib` | Yes | No Qt include; Little CMS stays private in [`ColourProfiles.h`](../src/io-lib/src/ColourProfiles.h), and no public header names libtiff, LibRaw or pugixml. |
| `widgets-lib` | Yes | `CurveView` and `HistogramView` draw what they are given, no effect; `AdjustmentsPanel` knows parameters, not effects. |
| `editor-lib` | Mostly | `MainWindow` decides policy of its own, see [Separation of Concerns](#separation-of-concerns). |
| `cli-lib` | Yes | `Grader` returns results; `main()` prints them. |

## Dependency Direction

Only downward, as `CLAUDE.md` asks: `editor-app` → `editor-lib` → `widgets-lib`, `io-lib` → `core-lib`, and `cli-app` → `cli-lib` → `io-lib` → `core-lib`.

| Library | Links | Includes from other libraries | Against the rule |
|---|---|---|---|
| `core-lib` | Threads | none | none |
| `io-lib` | `core-lib`; privately raw-converter, libtiff, pugixml, Little CMS | 9 of `core/` | none |
| `widgets-lib` | `core-lib`, Qt Widgets | 3 of `core/` | none |
| `editor-lib` | `core-lib`, `widgets-lib`, `io-lib`, Qt Widgets, Concurrent, Svg | 24 of `core/`, 9 of `imageio/`, 4 of `widgets/` | none |
| `cli-lib` | `core-lib`, `io-lib` | 1 of `core/`, 4 of `imageio/` | none |
| `editor-app`, `cli-app` | their library | `core/`, `imageio/`, `widgets/`, to compose | none: the composition roots |

## Interfaces and Abstractions

| Interface | Where | Judgement |
|---|---|---|
| `Effect` | [`Effect.h`](../src/core-lib/include/core/Effect.h) | Good: 17 effects, each a file; adding one is a file and a line in `makeStandardPipeline()`. `isIdentity()` lets the pipeline skip what changes nothing. |
| `Pipeline::render()` | [`Pipeline.h`](../src/core-lib/include/core/Pipeline.h) | Good: a `cancelled` callback, so the renderer and the camera match stop early without the engine knowing them. |
| `ImageReader` | [`ImageReader.h`](../src/io-lib/include/imageio/ImageReader.h) | Good: three implementations and a fake. `readCameraShot()` is a free function beside it, not behind it, so `matchCameraLook()` cannot be given a fake. |
| `SettingsStore` | [`SettingsStore.h`](../src/io-lib/include/imageio/SettingsStore.h) | Good: one implementation, the XMP sidecar, and a fake; where a sidecar lives is the store's to say, `location()`, so the document and the command line never build its name. |
| `core::match::Options` | [`CameraMatch.h`](../src/core-lib/include/core/CameraMatch.h) | Good: progress and stop as plain callbacks, so the search runs, and is tested, without Qt. |
| `StatusController` | [`Status.h`](../src/editor-lib/include/editor/Status.h) | Adequate: a thread-safe flag and a queued signal. One task at a time, which is all the window needs. |
| `AdjustmentsPanel` | [`AdjustmentsPanel.h`](../src/widgets-lib/include/widgets/AdjustmentsPanel.h) | Good: `addSectionWidget()`, `addSectionHeaderWidget()` and `addHeading()` let the window add to a section without the panel knowing why. `sectionTitles()` returns the titles in alphabetical order, not the order shown: *verified*, the probe got Chromatic Aberration first. |

## Separation of Concerns

**`MainWindow` holds policy.** It should lay the parts out and connect them; [`MainWindow.cpp`](../src/editor-lib/src/MainWindow.cpp), 713 lines, also decides:

- what a raw file is, `isRawFile()`, from a concrete `imageio::RawReader{}`, to choose which picture `matchCameraLook()` matches;
- how a matched grade meets the picture's own, merging it into `Document::settings()` in `showCameraLook()`;
- which file a picture may be saved to, refusing its own in `chooseImageFile()`, and which format was used last, in `QSettings`;
- the full flow of a long task: the controller, the `WaitingDialog`, the `QFutureWatcher`, the result.

Each is a few lines, but together they are rules no test reaches, see [Testability and Tests](#testability-and-tests).

Elsewhere, knowledge sits where it belongs: the document owns the files and their order, the renderer owns the passes, the engine owns the pixels and the search.

## SOLID

| Principle | Verdict | Where |
|---|---|---|
| Single responsibility | One violation | `MainWindow`, above. The effects, `Document`, `PreviewRenderer` and `Grader` each have one. |
| Open/closed | No violation found | An effect, a reader or an output format is added beside the others: `makeStandardPipeline()`, `makeStandardImageReader()`, `imageFormats()`. |
| Liskov substitution | No violation found | `FakeReader` and `FakeStore` stand in for the real ones in 28 document and command line tests. |
| Interface segregation | No violation found | `SettingsStore` has five functions, all used by the document; the command line uses two, `load()` and `loadFile()`. |
| Dependency inversion | Two small violations | `MainWindow::isRawFile()` builds a concrete `RawReader`; `matchCameraLook()` calls `imageio::readCameraShot()` directly. `Document`, `PreviewRenderer` and `Grader` are given their parts. |

## Coupling and Duplication

| Kept in step by hand | Weight | Where |
|---|---|---|
| The sRGB curve, written out four times | Medium: a fix to one, e.g. for values below 0, misses the others. | Encoding in [`Display.cpp`](../src/core-lib/src/Display.cpp) and [`ToneCurveEffect.cpp`](../src/core-lib/src/effects/ToneCurveEffect.cpp); decoding in `ToneCurveEffect.cpp` and [`CameraLook.cpp`](../src/editor-lib/src/CameraLook.cpp). |
| sRGB and CIE XYZ, one matrix and its inverse typed apart | Low: both from Lindbloom's tables, but one could be edited alone. | `XYZ_TO_SRGB` in [`Color.cpp`](../src/core-lib/src/Color.cpp), `SRGB_TO_XYZ` in [`CameraMatch.cpp`](../src/core-lib/src/CameraMatch.cpp). |
| The slider ids of the camera match | Low: a test checks each names a slider of the pipeline. | String literals in `standardStages()`, against the `Parameter`s of the effects. |
| The highlights and the shadows | Low, and documented: the same code but for three constants, each computing the area luminance again. | [`HighlightsEffect.cpp`](../src/core-lib/src/effects/HighlightsEffect.cpp), [`ShadowsEffect.cpp`](../src/core-lib/src/effects/ShadowsEffect.cpp). |
| Two luminances | None: Rec. 2020's for the working space, Rec. 709's for the histogram of the screen's image, each where it belongs. | [`Color.h`](../src/core-lib/include/core/Color.h), [`HistogramView.cpp`](../src/widgets-lib/src/HistogramView.cpp). |

## Testability and Tests

| Folder | Test cases | Untested |
|---|---|---|
| `core` | 52 | The look of the effects, by design, see the architecture's manual checks. |
| `io` | 23 | `TiffWriter` replacing a file when the rename fails, finding [1](#1-the-tiff-writer-deletes-the-file-it-replaces-before-replacing-it). |
| `editor` | 67 | `MainWindow`: no test constructs it. |
| `cli` | 7 | |

**The missing seam is `MainWindow`.** Its rules, the raw or TIFF choice of the camera match, the merging of a matched grade, the refusal to save over the picture, the actions enabled once a picture opens, can only be checked by hand. Moved into small classes of their own, each given a `Document` and a renderer, they would be tested as `Document` is.

**`matchCameraLook()` needs real raw files.** Its one end-to-end test is skipped without raw-converter's samples, as `readCameraShot()` cannot be faked.

## Implementation Issues

Ordered by severity. ThreadSanitizer's 85 reports all have Qt on one side: memory allocated by `QtConcurrent::run` on the main thread and read by a pooled thread, or handed between Qt's own image threads, through synchronisation an uninstrumented Qt hides. None has both accesses in our code; `core::parallelFor`, the renderer's generation counter and `core::match` are not among them.

### 1. The TIFF writer deletes the file it replaces before replacing it

*From reading.* [`TiffWriter::write()`](../src/io-lib/src/TiffWriter.cpp) calls `std::filesystem::remove(path)` and then `rename()`, on every system, for Windows' sake. If the rename then fails, the code also removes the temporary file: the old TIFF and the new one are both gone, against the architecture's claim that a failure never harms the file it would replace. [`XmpSidecar::save()`](../src/io-lib/src/XmpSidecar.cpp) renames without removing; one of the two is wrong on Windows.

### 2. An exception in a parallel chunk escapes while other chunks still run

*Verified.* A chunk of [`core::parallelFor()`](../src/core-lib/src/Parallel.cpp) that throws on the calling thread leaves `ThreadPool::run()` at once: the probe caught `out of memory` outside `parallelFor`. The chunks still running on other threads then count down a `Group` that lived on the unwound stack. A chunk that throws on a worker ends the program. Only `std::bad_alloc` can throw there today, on a picture too large for the memory.

### 3. The matched grade is fitted with the other sliders at their defaults

*From reading.* `core::match::fit()` starts from `Settings{}`, and [`MainWindow::showCameraLook()`](../src/editor-lib/src/MainWindow.cpp) applies the result over the picture's own settings. With the highlights, the HDR compression or the noise reduction already moved, the picture differs from the one whose difference the window reported.

### 4. Smaller

- `ParameterSlider::show(double)`, private, hides `QWidget::show()`: `slider.show()` does not compile; [`ParameterSlider.h`](../src/widgets-lib/include/widgets/ParameterSlider.h).
- **Match Camera Look…** on an open raw file reads it again from disk, about 3 seconds, though the document holds it; [`matchCameraLook()`](../src/editor-lib/src/CameraLook.cpp).
- A second picture asked for while one opens is dropped without a word; `Document::open()`.

## Recommendations

Ordered by value against effort; each is one pull request.

| # | Change | Fixes | Trade-off |
|---|---|---|---|
| 1 | Rename over the old TIFF without removing it first, and remove it only where the rename fails because it exists. | 1 | One more branch, tested on Windows. |
| 2 | Catch an exception in each chunk of `parallelFor`, keep the first, wait for every chunk, then throw it. | 2 | A few lines in the one place every effect goes through. |
| 3 | Give `core::match::fit()` the picture's settings to start from. | 3 | A match then depends on the picture's other sliders: say so in its result. |
| 4 | One sRGB curve, each way, and one sRGB-to-XYZ matrix, in `core::color`. | Duplication | None. |
| 5 | Move the camera match and the image save out of `MainWindow` into a class each, given the document and the renderer, and test them. Put `readCameraShot()` behind an interface beside `ImageReader`. | Separation, SOLID, testability | Two more classes; `MainWindow` back to layout and connections. |
| 6 | Keep the sections' order in `AdjustmentsPanel`, and rename `ParameterSlider::show()` to `showValue()`. | 4 and the probe | None. |
