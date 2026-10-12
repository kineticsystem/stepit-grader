---
name: reviewer
description: Review the design of StepIt Grader, or of one of its libraries, against SOLID, separation of concerns, library boundaries, dependency direction and testability, verify the suspected bugs with throwaway tests, and write or refresh docs/Review.md with a verdict, findings and recommendations. Use when asked to review, audit or judge the architecture or a library, to check the code against best practices, or to refresh the review after changes.
---

# Reviewer

You judge how StepIt Grader is built and write it down in `docs/Review.md`. How the code works is in `docs/ARCHITECTURE.md`, kept by the `architect` skill: read it first and do not repeat it; link to it.

The argument may name a part, e.g. `core-lib`, `editor-lib`, `the effects`. Without one, review the whole code base. A partial review updates only the sections it touches and says so in **Scope and Method**.

## 1. Read Before Judging

- **The house style**: load the `write-docs` skill. Match the existing `docs/Review.md`, if there is one.
- **The project's rules**: `CLAUDE.md` and `docs/ARCHITECTURE.md`'s Design Decisions. Judge a documented trade-off, but do not report it as a bug.
- **The code, whole**: every header and source file of the part, its `CMakeLists.txt`, its tests. A review that has not read a file cannot call it clean.
- **The facts that can be counted, counted**:
  - links between libraries: the `target_link_libraries` of each `src/*/CMakeLists.txt`;
  - includes between libraries: `grep -rhoE '#include "(core|imageio|editor)/' src/<lib> | sort | uniq -c`. `core-lib` and `io-lib` must include no Qt and nothing above them;
  - copies: `diff` the files that look alike, e.g. the effects that share a shape;
  - tests per folder of `src/tests`, and the parts with none.

## 2. The Questions

Answer each with a verdict and the evidence, file and function:

- **Structure and boundaries.** Does each library keep to what it owns? Does anything reach across, e.g. the engine knowing a file format, a widget knowing an effect, a public header leaking a third-party library?
- **Dependency direction.** Only downward: `editor-app` → `editor-lib` → `io-lib` → `core-lib`. A table of each include against it.
- **Interfaces and abstractions.** A table: interface (`Effect`, `ImageReader`, `SettingsStore`, ...), where, judgement.
- **Separation of concerns.** Each place where knowledge sits in the wrong part: rules in widgets, policy in `MainWindow`, rendering in the document.
- **SOLID.** A table, one row per principle, verdict and the place that proves it. Say "no violation found" when none is.
- **Coupling and duplication.** Rules kept in step by hand, e.g. the effects and `makeStandardPipeline()`, the ids and Lightroom's names, and copied code, each with a weight.
- **Testability and tests.** What is tested, what is not, and what seam is missing that makes it hard.
- **Implementation issues**, ordered by severity: threads, memory, colour accuracy, files a user could lose.
- **Recommendations**, ordered by value against effort, each small enough for one pull request, each naming the findings it fixes and its trade-off.

## 3. Verify What Can Be Verified

A suspected bug is reproduced before it is reported as *verified*:

1. Write a throwaway Google Test in the scratchpad, e.g. `ReviewProbeTest.cpp`.
2. Copy it into `src/tests/<folder>/`, add it to `src/tests/CMakeLists.txt`, and build: `cmake --build build`.
3. Run it: `QT_QPA_PLATFORM=offscreen build/bin/stepit-grader-tests --gtest_filter='ReviewProbe.*'`.
4. Remove it at once: delete the file, `git checkout -- src/tests/CMakeLists.txt`, rebuild, and check `git status`.

For threads, build once with `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` in `build/asan`, and with `-fsanitize=thread` in `build/tsan`, run under `setarch -R`; Qt is not instrumented, so report only races with a frame in our code. A probe that passes kills the suspicion: drop it, and tell the user what you had suspected. Mark everything else "from reading".

Also run the whole suite once and record the count and the time in **Scope and Method**.

## 4. Rules for Findings

- No finding without a file and a function. No praise without one either.
- Numbers, not adjectives.
- When a finding is fixed, remove it and renumber the rest: no "Fixed" marks.
- Do not fix anything while reviewing. The review is the deliverable; offer the fixes afterwards.

## 5. Finish

- Open the document with the date and the commit reviewed (`git rev-parse --short HEAD`).
- Render the diagrams with `~/.claude/skills/write-docs/scripts/render-mermaid.sh docs/Review.md` and look at them.
- Leave the changes uncommitted unless asked.
- Report to the user: the verdict in a few lines, the most serious findings, what was verified and what was read, and any suspicion that a probe disproved.
