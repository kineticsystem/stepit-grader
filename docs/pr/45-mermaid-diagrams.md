# Diagrams as SVG Images, from Mermaid Sources

Pull request [#45](https://github.com/kineticsystem/stepit-grader/pull/45), branch `mermaid-diagrams`.

## Goal

Every diagram of the documents in one style, the one of the `beautiful-mermaid-diagrams` skill: colours by role, compact, readable in light and dark mode. The three diagrams of `docs/ARCHITECTURE.md` move to `docs/diagrams/`: the libraries, the view models and a slider that moves.

## Decisions

**An SVG image, not a Mermaid block.** GitHub stretches a Mermaid block to the width of the page, so a narrow diagram becomes very tall. An image shows at the width we give it. The source stays beside it, in `docs/diagrams/<name>.mmd`.

**A hash, not a render, in CI.** `bin/render-diagrams.sh` writes the SHA-256 of the source into the first line of the SVG. `bin/check-diagrams.sh` compares the two, so CI needs neither Node.js nor Chrome. Whoever changes a `.mmd` renders it and commits both.

**One step of the Ubuntu job.** The check looks at the whole repository and needs no build, so it runs once, first, on Ubuntu, not on every system.

**Line breaks by hand.** The libraries were 1,086 px wide, with Mermaid wrapping the long details where it liked. Each detail now breaks where we choose, and the diagram is 891 px wide.

## Left Out

`modules/raw-converter`, whose diagrams change in its own repository; its pointer moves in a later pull request.

`docs/Review.md` has no diagram.
