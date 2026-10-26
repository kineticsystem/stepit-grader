# Pull Request Notes

Each pull request has a notes file here. It keeps what the pull request does not: what we discussed, what we chose, and why.

## The Notes

| Note | Pull request |
|---|---|
| [45-mermaid-diagrams.md](45-mermaid-diagrams.md) | [#45](https://github.com/kineticsystem/stepit-grader/pull/45) Diagrams as SVG images, from Mermaid sources |

## How to Write One

- One file per pull request: `<number>-<branch>.md`, e.g. `45-mermaid-diagrams.md`. Before it has a number, use the branch name.
- Sections: **Goal**, **Decisions**, **Left Out**, **Open Items**. Skip the ones that do not apply.
- Short sentences. One idea per paragraph. A diagram when it explains faster.
- Write it while the branch is open. Do not rewrite it after the merge.
- A change to the submodule `modules/raw-converter` is a pull request in its own repository, with its own note there; the note here links it.
