# StepIt Grader

## Table of Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
  - [Ubuntu 24.04](#ubuntu-2404)
  - [macOS](#macos)
  - [Windows 11](#windows-11)
- [Install StepIt Grader](#install-stepit-grader)
  - [Check out the Git Repository](#check-out-the-git-repository)
  - [Install the Libraries](#install-the-libraries)
  - [Build the Project](#build-the-project)
  - [Package for Linux](#package-for-linux)
  - [Use the Engine in Another Program](#use-the-engine-in-another-program)
- [Running the Application](#running-the-application)
  - [Open a Picture](#open-a-picture)
  - [Edit It](#edit-it)
  - [Match the Camera's Look](#match-the-cameras-look)
  - [Save the Settings](#save-the-settings)
  - [Save the Image](#save-the-image)
  - [Choose the Theme](#choose-the-theme)
  - [Keyboard Shortcuts](#keyboard-shortcuts)
- [Grade a Session in Three Steps](#grade-a-session-in-three-steps)
- [Grade from the Command Line](#grade-from-the-command-line)
- [The Tools](#the-tools)
- [The Sidecar](#the-sidecar)
- [Tests](#tests)
- [Troubleshooting](#troubleshooting)
- [Licences](#licences)

## Introduction

StepIt Grader is a desktop application, in C++ and Qt 6, for the tones and colours of a picture: the few sliders a photographer uses most, applied to the whole picture, live. It saves the edit, never the pixels: a sidecar of values next to the picture, which a command line tool can later apply, unchanged, to many pictures.

With StepIt Grader we can:

- open a TIFF, 8 or 16 bits or float, with its ICC profile, a DNG, or a camera raw file, e.g. a Canon CR2;
- set the white balance, by hand or by picking an area that should be white or grey, reduce the luminance and the colour noise of a high ISO, remove purple fringing, set the exposure, the contrast, the highlights, the shadows, the blacks, a tone curve, the detail, the structure, the vibrance, the saturation, the hue, the saturation and the luminance of eight bands of colour, an HDR compression of the highlights and the sharpness, and see the result as the slider moves;
- find the grade that makes raw files look like the JPEGs their camera made of them, and stop the search at any time with the best so far;
- zoom from the whole picture to 800%, and drag it around, with a histogram of the picture and its clipping always in sight;
- save the settings by itself in an XMP sidecar next to the picture, and open them again with it;
- save the graded picture at its full size, as a 16-bit TIFF in sRGB, Adobe RGB or ProPhoto RGB, or as a JPEG;
- grade any number of pictures from the command line with those settings, into 16-bit TIFFs in sRGB, Adobe RGB, ProPhoto RGB or linear Rec. 2020.

How it is built is described in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), and judged in [docs/Review.md](docs/Review.md).

![StepIt Grader in its dark theme: a macro of a wasp in the middle, the histogram and the tools on the right, the Light section open](docs/img/screenshot.png)

## Prerequisites

The editor is built with CMake 3.22 or later and a C++23 compiler, e.g. GCC 13, against these libraries:

| Library | Version | Where it comes from |
|---|---|---|
| [Qt](https://www.qt.io/download-open-source) | 6.5 or later, 6.11 tested | The Qt online installer. |
| [raw-converter](https://github.com/kineticsystem/raw-converter) | | A Git submodule, built with the editor. |
| [LibRaw](https://www.libraw.org/), [Little CMS](https://www.littlecms.com/), [libtiff](https://libtiff.gitlab.io/libtiff/), [pugixml](https://pugixml.org/) and [GoogleTest](https://github.com/google/googletest) | 0.21.4, 2.17, 4.7.0, 1.15, 1.15.2 | `./install-deps.sh`, which builds them from their own source releases, or the system's packages. |

`./install-deps.sh` needs a C and a C++ compiler, CMake, make, curl and zlib, which every system has.

> [!NOTE]
> The editor is built and tested on Ubuntu 24.04. The code is meant for macOS and Windows too, but has not been built there yet.

### Ubuntu 24.04

Install the build tools:

```bash
sudo apt install build-essential cmake ninja-build pkg-config curl zlib1g-dev
```

Install the latest Qt 6, with Qt Creator if you like, with the [Qt online installer](https://www.qt.io/download-open-source). Then tell CMake where Qt is, e.g.:

```bash
export Qt6_DIR=~/Qt/6.11.0/gcc_64/lib/cmake/Qt6
```

### macOS

1. Install Xcode, which brings the command line tools, from the [Apple Developer portal](https://developer.apple.com/download/): download the `.xip` file, double-click it, and drag Xcode into the Applications folder.
2. Install CMake and Ninja, e.g. with [Homebrew](https://brew.sh): `brew install cmake ninja pkg-config`.
3. Install the latest Qt 6 with the [Qt online installer](https://www.qt.io/download-open-source), and tell CMake where it is, e.g.:

   ```bash
   export Qt6_DIR=~/Qt/6.11.0/macos/lib/cmake/Qt6
   ```

### Windows 11

1. Download [MSYS2](https://www.msys2.org) and install it.
2. Open the MSYS2 **MINGW64** terminal, and update it, running this twice if it asks to restart the terminal:

   ```bash
   pacman -Syu
   ```

3. Install the compiler, the build tools and Qt 6:

   ```bash
   pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
       mingw-w64-x86_64-pkgconf mingw-w64-x86_64-zlib mingw-w64-x86_64-qt6 make curl git
   ```

## Install StepIt Grader

### Check out the Git Repository

Check out the repository with its submodule, raw-converter:

```bash
git clone --recurse-submodules git@github.com:kineticsystem/stepit-grader.git
```

If the submodule is missing, fetch it with:

```bash
git submodule update --init
```

### Install the Libraries

Build the libraries once, in about half a minute. Each is downloaded from its own project, at a fixed version, and installed into a folder of its own in `deps/`, which the build looks in first.

```bash
./install-deps.sh
```

There is one script per library in `scripts/`. A library already in `deps/` is skipped: delete its folder to build it again.

### Build the Project

Configure and build, from the repository's root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The programs are in `build/bin`: `stepit-grader`, the editor, `stepit-grader-cli`, the command line, and `stepit-grader-tests`. Build in `Release`: the engine is optimised in `Debug` too, but the rest of the program is not.

### Package for Linux

Make an AppImage, a single file that runs on Linux without installing anything:

```bash
./setup/linux/package-stepit-grader.sh
```

It builds the project, bundles Qt and every library with [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy), which `./install-deps.sh` downloads, and writes `build/linux-package/stepit-grader-<version>-x86_64.AppImage`. It holds both programs:

```bash
./stepit-grader-0.1.0-x86_64.AppImage                  # the editor
./stepit-grader-0.1.0-x86_64.AppImage --cli --help     # the command line
```

The AppImage runs on Linux systems at least as recent as the one it was made on.

### Use the Engine in Another Program

The engine and the panel of sliders build on their own, with nothing of `deps/`, for a program that grades its own pictures. Add the repository, e.g. as a submodule, and in its `CMakeLists.txt`:

```cmake
set(STEPIT_GRADER_ENGINE_ONLY ON)
set(STEPIT_GRADER_BUILD_WIDGETS ON)   # the sliders, with Qt 6 Widgets; OFF for the engine alone
add_subdirectory(modules/stepit-grader)
target_link_libraries(my-program PRIVATE stepit-grader::core stepit-grader::widgets)
```

[The Engine in Another Program](docs/ARCHITECTURE.md#the-engine-in-another-program) says how to use them.

## Running the Application

Start the editor, with a picture or without:

```bash
build/bin/stepit-grader ~/Pictures/IMG_0001.CR2
```

### Open a Picture

Open a picture with **File > Open**, or drop it on the window. **File > Open Recent** lists the last 10 pictures opened, the latest first; hover over one for its full path. **Clear**, at the bottom, empties the list, and a picture moved or deleted since goes from it when chosen. A raw file takes a few seconds to develop: about 2.5 seconds for the 21 megapixels of a Canon 5D Mark II. A picture that has a sidecar opens with its settings.

### Edit It

Move a slider on the right: its value shows beside its name, a blue bar on its line shows how far it moved from its default, and a double click on the slider or its name puts the default back, 0 for most. The sections start folded: a click on a section's title, e.g. **Light**, opens it, and another folds it away. The picture follows at once, on a reduced preview, then, once the sliders stop for 0.3 seconds, in full, which is what to look at at 100%.

The histogram above the sections shows the picture on the screen: how many pixels sit at each brightness, from black on the left to white on the right, in red, green and blue, with the luminance as a white line. A triangle in a top corner lights up when pixels are clipped at that end, a channel at black or at white, with their share of the picture below it. It follows every change, and the picture before the edit too.

To set the white balance from the picture, click the large eyedropper at the top of the **Color** section, or press `W`, then click an area that should be white or grey, e.g. a grey card or a white wall. The cursor shows the circle averaged, about 10 screen pixels across, whatever the zoom; its clipped pixels are left out. The temperature and the tint are set so the area turns neutral, and the eyedropper goes back to dragging. `Esc` cancels.

To compare with the picture before the edit, click the before and after button of the toolbar, the picture split in two, or press `\`. **Before** shows in the status bar while the picture is the one before the edit; the same button, or `\` again, goes back to the edit, at once. Moving a slider goes back to the edit too.

The toolbar, above the picture, holds the actions used most: open, save, before and after, and the zoom. Each is also in a menu, with its key. The white balance's eyedropper is at the top of the **Color** section, with its key, `W`, and in no menu.

Zoom with the wheel, around the cursor, or with the **View** menu; drag the picture with the mouse. A double click on the picture switches between the whole picture and 100% where clicked.

### Match the Camera's Look

A raw file looks darker and flatter in the editor than on the camera's screen: the camera shows the JPEG it made, with its own look, and the editor shows the raw as it is. **Edit > Match Camera Look…** finds the grade that gives the raw the camera's look: it tries values of the sliders until the raw, graded, looks like the camera's JPEG. What it matches depends on the picture open:

| Picture open | What is matched |
|---|---|
| A raw file or a DNG | The picture itself, against the JPEG it holds. Nothing to choose. When it opens, the status bar says so. |
| A TIFF | The raw files it was made from, which the editor asks for, starting in the TIFF's folder: a TIFF holds no camera JPEG, but a TIFF developed from raw files, linear and untouched, e.g. a focus stack merged, has the same tones as they have, so their grade fits it. |

A window shows how far it is: the stage, the trials and the difference so far. It takes about half a minute a picture. **Stop** ends it early, with the best grade found by then. The editor then shows the difference from the camera's JPEG before and after, in CIEDE2000, where below 1 cannot be seen and about 2 is seen side by side only, and asks whether to apply the grade. **Apply** sets the white balance, the tone, the tone curve and the colour mixer of the open picture; the other sliders stay as they are. The picture's sidecar keeps the grade, as any grade: **File > Load Settings…** applies it to another picture, and the command line grades many with it.

> [!IMPORTANT]
> Turn off the camera's Auto Lighting Optimizer for the raw files you match: it changes each JPEG by its content, so no grade can follow it.

### Save the Settings

The editor saves the settings by itself, next to the picture: `IMG_0001.xmp` for a raw file, `IMG_0001.CR2`, as raw editors name it, and `IMG_0001.tif.xmp` for a TIFF, `IMG_0001.tif`, so a raw file and a TIFF of the same name each keep their own. A TIFF whose sidecar was saved as `IMG_0001.xmp` by an earlier version opens with it, unless a raw file of the same name is beside it, and its next change is saved as `IMG_0001.tif.xmp`. The settings are saved 1 second after the last change, so a slider dragged writes the file once, when it stops. Opening another picture or closing the window saves at once what is still waiting, without asking. A picture opened and closed without a change gets no sidecar. When we open the picture again, it opens with its sidecar's settings, ready for another change.

The window's title shows `*` while the settings are not written yet: for the second after a change, or until the next change after a save that failed. A failed save is shown in the status bar and tried again at the next change; only when the edit would be lost, on closing the window or opening another picture, does the editor ask whether to go on without it. **File > Save Settings** saves at once.

**File > Load Settings…**, `Ctrl+L`, applies the settings of any sidecar to the picture open: one saved for another picture, or one another raw editor wrote. Every value comes from the file, those it lacks at their defaults. It is a change like any other, saved next to the picture a second later; **Edit > Revert to Opened** brings back the settings the picture had when it was opened.

**Edit > Revert to Opened** goes back to the settings the picture had when it was opened, even once a change has been saved over them, and **Edit > Revert to Default** to a picture with no edit, every slider at its default. Both are changes like any other, saved in turn. There is no undo: once the picture is closed, the settings it had when opened are gone.

### Save the Image

**File > Save Image…**, `Ctrl+Shift+E`, saves the picture with its grade, at its full size, while the settings stay in the sidecar. The type of file is chosen in the dialog, the last one chosen offered first:

| Type | What it is for |
|---|---|
| TIFF, 16 bits, sRGB | The default: what every program and screen assumes. |
| TIFF, 16 bits, Adobe RGB | Prints: wider in the greens and the cyans. |
| TIFF, 16 bits, ProPhoto RGB | More editing in another program: wider than any printer. |
| JPEG, 8 bits, sRGB | Sharing, at a high quality. |

A TIFF is compressed without loss and holds its ICC profile, as those of the command line do. The name offered is the picture's with `-graded`, next to it; the picture itself is never replaced. The picture is saved in the background, and the status bar says when it is done.

### Choose the Theme

**Settings > Theme** chooses the look: **System**, the desktop's own, the default; **Light**; or **Dark**, dark grey, easier on the eyes when judging a picture. The change is at once, and kept for the next time. On macOS, the menus show no icons, as their bar stays light whatever the theme; the toolbar keeps them.

### Keyboard Shortcuts

| Keys | What they do |
|---|---|
| `Ctrl+O` | Open a picture. |
| `Ctrl+S` | Save the settings now, rather than 1 second after the last change. |
| `Ctrl+L` | Load the settings of another sidecar. |
| `Ctrl+Shift+E` | Save the picture with its grade. |
| `Ctrl+Shift+R` | Revert every slider to its default. |
| `W` | Pick the white balance from an area; `Esc` cancels. |
| `\` | Show the picture before the edit, or after. |
| `Ctrl++`, `Ctrl+-` | Zoom in, zoom out. |
| `Ctrl+0` | Fit the picture to the window. |
| `Ctrl+1` | Show it at 100%. |

On macOS, `Cmd` replaces `Ctrl`.

## Grade a Session in Three Steps

The first time we grade pictures of a camera, set up as we shoot:

1. **Match.** Open a raw file of the session, or a TIFF made from raw files of it, and choose **Edit > Match Camera Look…**. For a TIFF, pick a few of the raw files it was made from.
2. **Apply.** Choose **Apply**: the picture's sidecar keeps the grade.
3. **Adjust.** Change what the picture needs on top, e.g. the exposure.

Every time after, with the same camera and settings:

1. **Load the grade.** Open the picture, choose **File > Load Settings…**, and pick the sidecar of a picture graded before, e.g. `IMG_0001.xmp`. It sets every slider, as that picture had them.
2. **Adjust.** Change what the picture needs on top.
3. **Grade the others.** Load the same sidecar for each, or grade them all from the command line with it: `stepit-grader-cli --settings IMG_0001.xmp`.

## Grade from the Command Line

`stepit-grader-cli` applies saved settings to pictures without a window, e.g. in a script or on a server: it reads each picture, applies the settings at full size, and writes a 16-bit TIFF with the picture's name into the output folder.

Grade every picture with the settings saved for one of them:

```bash
build/bin/stepit-grader-cli --settings ~/Pictures/IMG_0001.xmp --output ~/Pictures/graded ~/Pictures/*.CR2
```

Without `--settings`, each picture is graded with its own sidecar, or with no edit when it has none.

| Option | Default | What it does |
|---|---|---|
| `-s`, `--settings <file.xmp>` | each picture's own sidecar, `IMG_0001.xmp` for a raw file, `IMG_0001.tif.xmp` for a TIFF | The settings for every picture: any sidecar the editor saved. |
| `-o`, `--output <folder>` | required | Where the TIFFs go, made if missing: `<folder>/<picture's name>.tif`. |
| `-c`, `--colour-space <name>` | `srgb` | The TIFFs' colour space, with its ICC profile embedded: `srgb`, `adobe-rgb`, `prophoto` or `rec2020-linear`. sRGB is what every screen assumes; Adobe RGB and ProPhoto RGB keep the saturated colours sRGB clips, for prints; linear Rec. 2020 is the editor's own working space, for more processing. |
| `--compress` | off | Deflate, lossless: about half the size. |
| `--overwrite` | off | Replace the TIFFs already in the output folder. |

It never writes over a picture it reads, nor over a TIFF already there unless given `--overwrite`, nor over the TIFF of another picture of the same run, e.g. `a.CR2` and `a.tif`. A picture that cannot be read or written is reported, and the others are graded. It prints each TIFF written, and ends with:

| Exit status | Meaning |
|---|---|
| `0` | Every picture is graded. |
| `1` | Some are not: each is reported, with the reason. |
| `2` | The command line is wrong, or the settings cannot be read: nothing is graded. |

A raw file of 21 megapixels takes about 5 seconds, most of it to develop.

## The Tools

The tools as the panel shows them, from top to bottom; [The Effects](docs/ARCHITECTURE.md#the-effects) gives the order they run in, e.g. the sharpening last. Each changes the whole picture, never one area chosen by hand.

| Section | Tool | Range | What it does |
|---|---|---|---|
| Color | Temperature | `-100` to `100` | Warmer above 0, cooler below, from the picture's own white balance. |
| | Tint | `-100` to `100` | More magenta above 0, more green below. |
| Light | Exposure | `-5.00` to `5.00` | Stops of light: `+1.00` is twice as bright. |
| | Contrast | `-100` to `100` | Spreads the tones away from middle grey, or toward it. |
| | Highlights | `-100` to `100` | Darkens, or brightens, the bright areas only. |
| | Shadows | `-100` to `100` | Brightens, or darkens, the dark areas only. |
| | Blacks | `-100` to `100` | Crushes the darkest tones to black, or lifts black to a dark grey. |
| | Vibrance | `-100` to `100` | Saturates the dull colours more than the vivid ones. |
| | Saturation | `-100` to `100` | Saturates every colour; `-100` is black and white. |
| Tone Curve | Curve Shadows, Curve Darks, Curve Lights, Curve Highlights | `-100` to `100` | Raises, or lowers, one region of the tones, from the darkest to the brightest, with a smooth curve that never goes down. The colours stay, and white and above are untouched. The curve is drawn above the sliders: the tones before the edit along the bottom, after it up the side, the regions' borders dotted. |
| | Shadows Split, Midtones Split, Highlights Split | `5` to `45`, `25` to `75`, `55` to `95` | Where one region ends and the next begins, in tones as the screen shows them; `25`, `50` and `75` by default. |
| Color Mixer | Red Hue … Magenta Hue | `-100` to `100` | Turns the colours of one band toward the next band, e.g. the reds toward orange above 0, toward magenta below. Eight bands: red, orange, yellow, green, aqua, blue, purple and magenta. |
| | Red Saturation … Magenta Saturation | `-100` to `100` | Saturates the colours of one band; `-100` makes them grey. |
| | Red Luminance … Magenta Luminance | `-100` to `100` | Brightens, or darkens, the colours of one band, by up to a stop. Greys are left alone by every band. |
| Effects | Detail | `-100` to `100` | The contrast of the fine texture, a few pixels across, e.g. hairs. |
| | Structure | `-100` to `100` | The contrast of the medium shapes, e.g. a leg against a body. |
| | Sharpness | `0` to `150` | Sharpens the finest edges. Look at it at 100%. |
| Noise Reduction | Luminance | `0` to `100` | Smooths the grain of the luminance a high ISO makes, in the shadows and the highlights alike, and keeps the edges. |
| | Luminance Detail | `0` to `100` | How much of the fine detail the luminance smoothing keeps; `50` by default. |
| | Color | `0` to `100` | Removes the coloured blotches of a high ISO, mostly in the shadows, without colour bleeding across edges. The luminance, and so the detail, stays. |
| Chromatic Aberration | Purple Amount | `0` to `20` | Takes the colour out of purple fringes, the purple edges a lens draws along strong contrasts. Higher also searches a wider band beside the edges. |
| | Purple Hue Low, Purple Hue High | `0` to `100` | The hues taken for fringes, from blue (`0`) through purple to magenta (`100`); `30` to `70` by default. A purple area away from edges keeps its colour. |
| HDR | HDR Compression | `0` to `100` | Fits the highlights brighter than white back into the picture, up to 4 stops above it, with a smooth roll-off instead of a clip. |
| | Compression Point | `0` to `100` | Where the roll-off starts: from middle grey (`0`) to just below white (`100`). The tones below are untouched. |
| | Local Contrast | `0` to `100` | At `100`, compresses the brightness of whole areas and keeps the texture within them, e.g. the clouds of a bright sky; at `0`, every pixel on its own. |

The picture is edited in linear Rec. 2020 with 32-bit floats: no colour is clipped and no tone is lost between the tools, only on the screen.

## The Sidecar

The settings are saved in an XMP file next to the picture, `IMG_0001.xmp` for a raw file and `IMG_0001.tif.xmp` for a TIFF, each value an attribute of the `crs` namespace, the one most raw editors keep their settings in, under the name they give it:

```xml
<x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="stepit-grader">
 <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
  <rdf:Description rdf:about="" xmlns:crs="http://ns.adobe.com/camera-raw-settings/1.0/"
    crs:ProcessVersion="11.0" crs:HasSettings="True" crs:IncrementalTemperature="0"
    crs:IncrementalTint="0" crs:Exposure2012="-0.35" crs:Contrast2012="0"
    crs:Highlights2012="0" crs:Shadows2012="+45" crs:Blacks2012="0" crs:Texture="0"
    crs:Clarity2012="0" crs:Vibrance="0" crs:Saturation="0" crs:Sharpness="0" />
 </rdf:RDF>
</x:xmpmeta>
```

What that means in practice:

- **The editor reads the sidecars of other raw editors** that use the same namespace: the values of its tools, whether written as attributes or as elements. Their other settings, e.g. a point tone curve, `ToneCurvePV2012`, or lens corrections, are ignored.
- **Saving keeps the rest of the file**, e.g. a rating or keywords: only the values of the tools are replaced.
- **The same numbers do not give the same picture** in another editor: the names and the ranges are shared, the processing is each editor's own.
- **The HDR sliders are the editor's own**, `HDRToneCompression`, `HDRCompressionPoint` and `HDRLocalContrast`: other editors ignore them.
- **The temperature and the tint are relative**, `IncrementalTemperature` and `IncrementalTint`, the same for a raw file as for a TIFF.

## Tests

The tests are one program, `stepit-grader-tests`. Run them with CTest:

```bash
ctest --test-dir build
```

or the program itself, which also takes a filter:

```bash
build/bin/stepit-grader-tests --gtest_filter='EffectsTest.*'
```

Four tests read camera raw samples, which are not in the repository: reading a CR2, a DNG and a CR2's camera JPEG, and matching a camera's look. Download them once, about 160 MB, into the submodule:

```bash
modules/raw-converter/bin/fetch-samples.sh
```

Without them, those four tests are skipped.

## Troubleshooting

**CMake says `Could not find a package configuration file provided by "Qt6"`.** It does not know where Qt is: set `Qt6_DIR`, see [Prerequisites](#prerequisites).

**CMake says `Package 'libraw_r', required by 'virtual:world', not found`, or does not find TIFF or pugixml.** The libraries are neither in `deps/` nor in the system: run `./install-deps.sh`.

**The sliders are slow.** Build in `Release`.

**A picture opens with no edit, and a warning says its settings cannot be read.** Its sidecar is damaged, e.g. cut short by a full disk or edited by hand. The first change replaces it with a new one; the damaged one stays beside it, e.g. `IMG_0001.xmp.damaged`, to recover by hand.

## Licences

StepIt Grader is released under the MIT licence, see [`LICENSE`](LICENSE). The software it uses:

| Software | Licence | Role |
|---|---|---|
| [Qt](https://www.qt.io) | LGPL 3 | The user interface, linked dynamically. |
| [raw-converter](https://github.com/kineticsystem/raw-converter) | MIT | Develops the raw files. |
| [LibRaw](https://www.libraw.org/) | LGPL 2.1 or CDDL 1.0 | Decodes and demosaics the raw files, in raw-converter, linked dynamically. |
| [Little CMS](https://www.littlecms.com/) | MIT | Converts the colours of a TIFF from its ICC profile. |
| [libtiff](https://libtiff.gitlab.io/libtiff/) | libtiff licence, BSD-like | Reads and writes the TIFF files, linked dynamically. |
| [pugixml](https://pugixml.org/) | MIT | Reads and writes the XMP sidecars, linked statically. |
| [zlib](https://zlib.net/) | zlib licence | Deflate, for libtiff: the system's. |
| [GoogleTest](https://github.com/google/googletest) | BSD 3-Clause | The tests. |
| [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) | MIT | Makes the Linux AppImage; not in the AppImage itself. |
