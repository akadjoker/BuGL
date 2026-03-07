# Contributing to BuGL

Thanks for contributing.

## Development Setup

Build:

```bash
cmake -S . -B build
cmake --build build -j
```

Run a script:

```bash
./bin/main scripts/tutor_6.bu
```

## Contribution Areas

- BuLang bindings (`main/src/*bindings*`, OpenGL split files, STB wrappers)
- Runtime stability and GC safety
- API consistency and error messages
- Tutorials and demos under `scripts/`
- Documentation improvements

## Coding Guidelines

- Keep API names consistent with existing style.
- Prefer small focused commits and PRs.
- Avoid breaking existing script behavior without clear migration notes.
- For bindings, validate arguments and return explicit errors.
- For hot paths, avoid unnecessary data copies.

## Pull Request Checklist

- Project builds successfully.
- New/changed scripts run without runtime errors.
- Behavior changes are documented in `README.md` when relevant.
- Added or updated minimal example when adding new API.
- No unrelated refactors mixed into the same PR.

## Bug Reports

Please include:

- OS and GPU/driver
- Exact script used (minimal repro)
- Expected vs actual behavior
- Console/runtime output

## Feature Requests

Describe:

- Use case (teaching, prototyping, production path)
- Proposed API shape
- Minimal sample showing expected usage
