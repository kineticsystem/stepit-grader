# TODO

## A Faster Linker on Windows

**Status:** to discuss. Nothing is changed yet.

### The Problem

On Windows, MinGW links with its default linker, GNU `ld` (BFD): the project chooses none. It is slow there, single-threaded, and reads every input in full. The programs link large static libraries (`core-lib`, `io-lib`, `widgets-lib`, `editor-lib`, `librawconv.a`), and every change to `core-lib` links the editor, the command line and the tests again. In Debug, with `-g`, it also copies and merges the debug information, and a link can take tens of seconds against a second or two on Linux.

Compiling is not the problem: in the CI, the build of StepIt Grader and its tests takes about as long on Windows as on Ubuntu, 1:49 against 1:52 on runners of the same size. What is slow there is shell work: `./configure` in `install-deps.sh` and the DLL scan of `setup/win`, both made of thousands of short processes, which MSYS2 starts slowly.

### The Proposal

Link with LLVM's `lld` on MinGW, which supports MinGW's executables and is often five to ten times faster there. `mold` and `gold` cannot make Windows executables; `lld` is the only fast choice.

- `CMakeLists.txt`: with MinGW, check that `-fuse-ld=lld` links (`check_linker_flag()`), and use it for the programs when it does; fall back to `ld` when `lld` is not installed. An option, e.g. `STEPIT_GRADER_USE_LLD`, on by default, to turn it off.
- The README's MSYS2 packages: add `mingw-w64-x86_64-lld`.
- The CI's Windows job: install `mingw-w64-x86_64-lld`.

### Try It First

In the MSYS2 MINGW64 terminal, in a new build folder:

```bash
pacman -S mingw-w64-x86_64-lld
cmake -S . -B build-lld -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld
cmake --build build-lld
```

Then change one source of `core-lib`, build again, and compare the time with `ld` in `build/`: `time cmake --build build`.

### Open Questions

- How much faster is the link on a real Windows machine, in Debug and in Release?
- Do the tests and the packages still work with `lld`? The CI would tell.
- Is `lld` worth it on Linux and macOS too? On Linux, `mold` or `lld` would also link faster than `ld`; on macOS, Apple's linker is already fast.

### Also Worth Doing on Windows

- Exclude the repo and `C:\msys64` from Microsoft Defender's real-time scanning, which scans every file the compiler writes.
- `ccache`, `mingw-w64-x86_64-ccache`, with `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache`, for rebuilds.
- `setup/win`: start fewer processes in the DLL scan, and reuse the CI's build instead of building the project a second time: about three or four minutes a run.
