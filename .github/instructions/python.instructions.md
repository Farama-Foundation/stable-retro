---
description: "Use when changing Stable Retro Python packages, Python tests, packaging metadata, or public Python APIs."
applyTo: "stable_retro/**/*.py,retro/**/*.py,tests/test_python/**/*.py,setup.py,pyproject.toml"
---

# Python Guidelines

- Treat `stable_retro` as the implementation package and `retro` as its compatibility shim.
- Preserve Python 3.10 compatibility and follow the repository's Black, isort, Flake8, and pyupgrade configuration.
- Keep Python wrappers consistent with native bindings exposed from `src/retro.cpp`.
- Add or update focused tests under `tests/test_python/` for behavior changes.
- Run `scripts/test-python.sh <test-path>` while iterating and `scripts/test-python.sh` before completion.
- Run `python -m pre_commit run --files <changed-files>` for formatting and static checks.
