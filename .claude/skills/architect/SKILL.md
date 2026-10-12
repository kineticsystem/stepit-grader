---
name: architect
description: Describe how StepIt Grader is built and keep docs/ARCHITECTURE.md true - the libraries, the engine and its effects, the readers and the sidecar, the editor's widgets, document and renderer, threads, tests, how to extend it and the design decisions. Use when asked to document, explain or refresh the architecture, after a change that moves a responsibility, adds an effect, a library or a public interface, or before planning a refactoring that needs the current picture.
---

# Architect

You describe how StepIt Grader is built in [`docs/ARCHITECTURE.md`](../../../docs/ARCHITECTURE.md): what each part owns, what it must never do, and the one detail that is easy to break. The document is for a developer who has read the README, built the project and edited a picture once, and wants to know where to start a change.

The architecture describes; it does not judge. Judgements, findings and recommendations go in `docs/Review.md`, written by the `reviewer` skill. Link to it where a weakness explains why something is as it is.

The argument may name a part, e.g. `core-lib`, `the effects`, `the sidecar`. Without one, refresh the whole document.

## 1. Read Before Writing

- **The house style**: load the `write-docs` skill and follow it: voice, shape, tables, Mermaid front matter, rendering check. The existing `docs/ARCHITECTURE.md` is the model to match.
- **The code, whole, for the part you describe**: every header in `src/<lib>/include/` and the `.cpp` files behind the parts you name, the library's `CMakeLists.txt`, and its tests in `src/tests/<folder>/`. Do not describe a class from its name.
- **What it talks to**: the includes in and out of the part, e.g. `grep -rhoE '#include "(core|imageio|editor)/' src/<lib> | sort | uniq -c`; the `target_link_libraries` of every `CMakeLists.txt`; who calls its public classes.
- **The project's own rules**: `CLAUDE.md`. When the code and a rule disagree, describe the code and tell the user about the rule.

## 2. The Document

Keep the sections of the existing document, in this order, adding one only when a part needs it:

1. **Introduction**, with the one idea in bold: *an edit is a set of named values, never pixels*. If a change breaks that idea, say so in the document and to the user.
2. **The Big Picture**: the libraries and the programs, as a diagram, and a table of the programs.
3. **The Libraries**: one table, `| Library | Owns | Links | Must never |`, from `CMakeLists.txt`, not from memory.
4. **The Engine**: the image and the working space, `Effect` and `Pipeline`, a table of the effects with their parameter ids, what they share, threads and memory with measured times.
5. **Reading and Saving**: the readers, the XMP sidecar.
6. **The Editor**: the widgets and their signals, the document, the renderer, the main window's connections, `main()`.
7. **A Slider Moves**: the main flow as a sequence diagram, at most four participants.
8. **Threads**: a table of what runs where and how its result comes back to the main thread.
9. **Tests**: a table per folder of `src/tests`, with the number of test cases (`grep -rhE '^(TEST|TEST_F|TEST_P)\(' <folder> | wc -l`), how to run them, and what has no test.
10. **How to Extend StepIt Grader**: a `| To... | Change... |` table.
11. **Design Decisions and Trade-offs**: a bold decision, the reason, the price.

Rules:

- Real names and numbers: classes, files, signals, parameter ids, line counts, test counts, milliseconds measured on a real picture. Link files relatively from `docs/`.
- Check every claim you have not read in the code, or remove it.
- Present tense, how things are. History belongs in git.
- Update the table of contents with every heading.
- Render every new or changed diagram with `~/.claude/skills/write-docs/scripts/render-mermaid.sh docs/ARCHITECTURE.md` and look at each image at full size, light and dark.

## 3. Keep It True

When code changes an interface, an effect, a library's links, a thread or a test folder, update `docs/ARCHITECTURE.md`, the README's tables and `CLAUDE.md` in the same change. If the change fixes a finding of `docs/Review.md`, tell the user so the review can drop it.

## 4. Finish

- Leave the changes uncommitted unless asked.
- Report to the user: what changed in the document, anything in the code that contradicts `CLAUDE.md`, and anything you could not check.
