# Contributing

Thank you for considering a contribution to Jenuroix Engine!

---

## Getting Started

1. Fork the repository on GitHub.
2. Clone your fork:
   ```bash
   git clone https://github.com/your-username/jenuroix-engine.git
   cd jenuroix-engine
   ```
3. Create a feature branch:
   ```bash
   git checkout -b feature/my-improvement
   ```
4. Build and run the examples to verify your baseline (see [BUILDING.md](docs/BUILDING.md)).

---

## Code Style

- **Language**: C++17.
- **Indent**: 4 spaces. No tabs.
- **Braces**: K&R style (opening brace on the same line).
- **Names**: `snake_case` for variables and functions, `PascalCase` for types and classes.
- **Comments**: English only.
- **Headers**: prefer `#pragma once`.
- Keep each example self-contained — no cross-example dependencies.
- Prefer single-header third-party libraries to keep the dependency graph small.

---

## Submitting Changes

1. Make sure all existing examples still compile and run.
2. If your change affects the public API, update the relevant example and `docs/API.md`.
3. Add an entry to `CHANGELOG.md` under `[Unreleased]`.
4. Commit with a clear message:
   ```
   feat: add ScriptWobble helper to 20_scripts.cpp
   fix: correct sphere radius in 04_collision example
   docs: document Audio::setPosition in API.md
   ```
5. Push and open a Pull Request. Fill in the PR template.

---

## Adding an Example

- Place new examples in `examples/` with the next sequential number: `24_xxx.cpp`.
- Keep the file self-contained (`#include "engine.h"` only, plus optional subsystem headers).
- Begin the file with a block comment explaining what the example demonstrates and its controls.
- Add a row to the Examples table in `README.md`.

---

## Reporting Bugs

Open a GitHub Issue and include:
- OS and compiler version.
- Steps to reproduce.
- The full compiler / runtime error output.
- Which example (or your own code) triggers the bug.

---

## License

By submitting a pull request you agree that your contribution will be
released under the project's [MIT License](LICENSE).
