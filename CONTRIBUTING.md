# Stable Retro Contribution Guidelines

Stable Retro welcomes bug reports, bug fixes, documentation improvements, new features, game integrations, and emulator platform work. Open an issue before beginning a large feature or core integration so its design, licensing, and platform support can be discussed early.

## Issue reports

Use the relevant GitHub issue form and include:

- Operating system and architecture
- Python version
- Stable Retro version or git commit
- Emulator platform and core, when relevant
- Reproduction steps, expected behavior, and complete error output

Do not upload commercial ROMs, BIOS files, encryption keys, or other copyrighted assets to issues or pull requests.

## Development setup

On Debian or Ubuntu, install the native dependencies:

```bash
sudo apt-get update
sudo apt-get install -y cmake capnproto zlib1g-dev build-essential pkg-config libzip-dev libbz2-dev xvfb python3-opengl libgl1-mesa-dev libglu1-mesa-dev
```

Install Stable Retro and its development tools:

```bash
python -m pip install -e '.[dev]'
```

## Testing and style

Run Python tests with:

```bash
scripts/test-python.sh
```

Run native tests with:

```bash
CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/test-cpp.sh
```

Run repository checks on changed files with:

```bash
python -m pre_commit run --files <changed-files>
```

Python is formatted and checked by the tools configured in `.pre-commit-config.yaml`. C++ follows `.clang-format`. The legacy `scripts/lint.sh` command rewrites files in place; inspect your diff after using it.

## Integration work

- For a new game integration, follow `docs/integration.md`.
- For a new emulator system or libretro core, account for licensing, host support, manifest mapping, packaging, native tests, and documentation. The repository workflow is captured in `.github/skills/add-emulator-platform/SKILL.md`.
- Test fixtures must be freely redistributable and include clear provenance.
