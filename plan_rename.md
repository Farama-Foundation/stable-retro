# Plan: Rename Package from `retro` to `stable_retro`

## Overview

This plan outlines the steps to rename the Python package from `retro` to `stable_retro` while maintaining backward compatibility with a deprecation warning for the old import name.

## Current State Analysis

The project is currently structured as:
- **Package name on PyPI**: `stable-retro` (already correct)
- **Python import name**: `retro` (needs to change to `stable_retro`)
- **Main package directory**: `retro/` (needs to rename to `stable_retro/`)
- **C extension module**: `retro._retro` (needs to become `stable_retro._retro`)

## Goals

1. Users can do `import stable_retro` (new preferred way)
2. Users can still do `import retro` but with a deprecation warning
3. All internal references updated to use `stable_retro`
4. Build system updated to compile C extensions with correct names
5. Documentation updated to reflect new import name
6. Tests updated to use new import name

## Implementation Strategy

### Phase 1: Directory and File Renaming

#### 1.1 Rename Main Package Directory
- Rename [`retro/`](retro/) → [`stable_retro/`](stable_retro/)
- This is the primary package directory containing all Python modules

#### 1.2 Update C Extension Target Names
Files to modify:
- [`setup.py`](setup.py:100)
  - Change `Extension("retro._retro", ...)` → `Extension("stable_retro._retro", ...)`
  - Update packages list to use `stable_retro` prefix
  - Update package_data keys to use `stable_retro` prefix
  - Update VERSION_PATH to use `stable_retro/VERSION.txt`
  - Update make target from `"make", jobs, "retro"` to `"make", jobs, "stable_retro"`

- [`CMakeLists.txt`](CMakeLists.txt:34)
  - Update VERSION.txt path: `retro/VERSION.txt` → `stable_retro/VERSION.txt`
  - Update any build targets that reference "retro"

### Phase 2: Update Internal Imports

#### 2.1 Core Package Files
Update all internal imports in the new `stable_retro/` directory using the pattern `import stable_retro as retro`:

- [`stable_retro/__init__.py`](retro/__init__.py)
  - Line 4: `import retro.data` → `import stable_retro.data`
  - Line 5: `from retro._retro import ...` → `from stable_retro._retro import ...`
  - Line 6: `from retro.enums import ...` → `from stable_retro.enums import ...`
  - Line 7: `from retro.retro_env import ...` → `from stable_retro.retro_env import ...`
  - Line 31: `retro.data.init_core_info(...)` → `stable_retro.data.init_core_info(...)`

- [`stable_retro/retro_env.py`](retro/retro_env.py)
  - Lines 9-10: Change to:
    ```python
    import stable_retro as retro
    import stable_retro.data
    ```
  - Line 251: `from retro.rendering import ...` → `from stable_retro.rendering import ...`
  - All other `retro.` references will continue to work via the alias

- [`stable_retro/data/__init__.py`](retro/data/__init__.py)
  - Line 8: `from retro._retro import ...` → `from stable_retro._retro import ...`
  - Line 9: `from retro._retro import data_path` → `from stable_retro._retro import data_path`
  - Lines 437, 501: Change to:
    ```python
    import stable_retro as retro
    ```
  - All `retro.` references will continue to work via the alias

#### 2.2 Scripts and Examples
Update imports in all subdirectories using `import stable_retro as retro` pattern where the `retro` name is used:

**Scripts** ([`stable_retro/scripts/`](retro/scripts/)):
- [`import_path.py`](retro/scripts/import_path.py:6): `import retro.data` → `import stable_retro.data`
- [`import_sega_classics.py`](retro/scripts/import_sega_classics.py:13): `import retro.data` → `import stable_retro.data`
- [`playback_movie.py`](retro/scripts/playback_movie.py:15): `import retro` → `import stable_retro as retro`

**Import modules** ([`stable_retro/import/`](retro/import/)):
- [`__main__.py`](retro/import/__main__.py:1): `from retro.scripts.import_path import ...` → `from stable_retro.scripts.import_path import ...`
- [`sega_classics.py`](retro/import/sega_classics.py:1): `from retro.scripts.import_sega_classics import ...` → `from stable_retro.scripts.import_sega_classics import ...`

**Examples** ([`stable_retro/examples/`](retro/examples/)):
- [`interactive.py`](retro/examples/interactive.py:18): `import retro` → `import stable_retro as retro`
- [`random_agent.py`](retro/examples/random_agent.py:3): `import retro` → `import stable_retro as retro`
- [`retro_interactive.py`](retro/examples/retro_interactive.py:3-4): `import retro` → `import stable_retro as retro` and `from stable_retro.examples.interactive import ...`
- [`discretizer.py`](retro/examples/discretizer.py:8): `import retro` → `import stable_retro as retro`
- [`trivial_random_agent.py`](retro/examples/trivial_random_agent.py:1): `import retro` → `import stable_retro as retro`
- [`trivial_random_agent_multiplayer.py`](retro/examples/trivial_random_agent_multiplayer.py:1): `import retro` → `import stable_retro as retro`
- [`ppo.py`](retro/examples/ppo.py:18): `import retro` → `import stable_retro as retro`
- [`determinism.py`](retro/examples/determinism.py:11): `import retro` → `import stable_retro as retro`
- [`brute.py`](retro/examples/brute.py:19): `import retro` → `import stable_retro as retro`

**Testing** ([`stable_retro/testing/`](retro/testing/)):
- [`tools.py`](retro/testing/tools.py:5): `import retro.data` → `import stable_retro.data`

#### 2.3 Root-level Scripts
Update imports in [`scripts/`](scripts/) directory:
- [`import.py`](scripts/import.py:2): `from retro.scripts.import_path import ...` → `from stable_retro.scripts.import_path import ...`
- [`import_sega_classics.py`](scripts/import_sega_classics.py:2): `from retro.scripts.import_sega_classics import ...` → `from stable_retro.scripts.import_sega_classics import ...`
- [`playback_movie.py`](scripts/playback_movie.py:2): `from retro.scripts.playback_movie import ...` → `from stable_retro.scripts.playback_movie import ...`

**Note:** These scripts use `from ... import ...` syntax, so the alias pattern is not needed here.

### Phase 3: Create Backward Compatibility Layer

Create a new `retro/` directory with a compatibility shim:

**File: `retro/__init__.py`** (new compatibility module)
```python
"""
Backward compatibility module for retro -> stable_retro migration.

This module provides a compatibility layer that allows 'import retro' to continue
working while warning users about the deprecation.
"""
import sys
import warnings

# Issue deprecation warning
warnings.warn(
    "The 'retro' package name is deprecated and will be removed in a future version. "
    "Please use 'import stable_retro' instead of 'import retro'.",
    DeprecationWarning,
    stacklevel=2
)

# Import and re-export everything from stable_retro
from stable_retro import *  # noqa: F401, F403
import stable_retro as _stable_retro

# Make sure submodules are accessible
sys.modules['retro'] = sys.modules['stable_retro']
sys.modules['retro.data'] = _stable_retro.data
sys.modules['retro.scripts'] = _stable_retro.scripts
sys.modules['retro.examples'] = _stable_retro.examples
sys.modules['retro.import'] = _stable_retro.import
sys.modules['retro.testing'] = _stable_retro.testing
sys.modules['retro._retro'] = _stable_retro._retro
sys.modules['retro.enums'] = _stable_retro.enums
sys.modules['retro.retro_env'] = _stable_retro.retro_env
sys.modules['retro.rendering'] = _stable_retro.rendering

# Re-export commonly used items
__version__ = _stable_retro.__version__
__all__ = _stable_retro.__all__
```

**Update `setup.py`** to include the compatibility layer:
```python
packages=[
    "retro",  # Compatibility shim
    "stable_retro",
    "stable_retro.data",
    "stable_retro.data.stable",
    "stable_retro.data.experimental",
    "stable_retro.data.contrib",
    "stable_retro.scripts",
    "stable_retro.import",
    "stable_retro.examples",
    "stable_retro.testing",
],
```

### Phase 4: Update Configuration Files

#### 4.1 Build Configuration
- [`MANIFEST.in`](MANIFEST.in:4): Update `retro/cores` reference → `stable_retro/cores`
- [`setup.cfg`](setup.cfg:6): Update `norecursedirs` if it references retro

#### 4.2 Documentation Configuration
- [`docs/conf.py`](docs/conf.py): Update any references to `retro` module paths

### Phase 5: Update Documentation

Update all documentation files in [`docs/`](docs/):

#### 5.1 Getting Started Guide
[`docs/getting_started.md`](docs/getting_started.md):
- Line 16: `import retro` → `import stable_retro as retro` (or just `import stable_retro`)
- Line 35: `python3 -m retro.examples.interactive` → `python3 -m stable_retro.examples.interactive`
- Line 50: `python3 -m retro.examples.random_agent` → `python3 -m stable_retro.examples.random_agent`
- Line 60: `python3 -m retro.examples.brute` → `python3 -m stable_retro.examples.brute`
- Line 72: `python3 -m retro.examples.ppo` → `python3 -m stable_retro.examples.ppo`
- Line 82: `import retro` → `import stable_retro as retro` (or just `import stable_retro`)
- Line 95: `python3 -m retro.import` → `python3 -m stable_retro.import`

Add a note about backward compatibility:
```markdown
## Migration from gym-retro or old stable-retro

If you're migrating from gym-retro or an older version of stable-retro, note that the package import name has changed:

**Old way (deprecated):**
```python
import retro  # Still works but shows deprecation warning
```

**New way (recommended):**
```python
import stable_retro as retro  # Easy migration - keeps your code working
# or
import stable_retro  # Use the full new name
```

The old `import retro` syntax will continue to work with a deprecation warning to ease migration.
Using `import stable_retro as retro` allows you to migrate with minimal code changes.
```

#### 5.2 Python API Documentation
[`docs/python.md`](docs/python.md):
- Update all `retro.` references to `stable_retro.`
- Lines 5, 8, 12, 15, 18, 24, 27, 31, 38, 44, 62, 64, 77, 79, 82, 86, 105: Change `retro.` → `stable_retro.`
- Add examples showing both `import stable_retro` and `import stable_retro as retro` patterns

#### 5.3 Integration Guide
[`docs/integration.md`](docs/integration.md):
- Line 87: `retro.RetroEnv` → `stable_retro.RetroEnv`
- Line 243: Update note about telling `retro` → `stable_retro`
- Lines 246, 253, 256, 257, 265: Update all `retro` references

#### 5.4 Index/Main Documentation
[`docs/index.md`](docs/index.md):
- Line 42: `import retro` → `import stable_retro`
- Lines 88-89: Update `retro.` references
- Line 108: Update `retro.State` reference

#### 5.5 Development Guide
[`docs/developing.md`](docs/developing.md):
- Update repository references if needed (already correct as stable-retro)

### Phase 6: Update Tests

Update all test files in [`tests/`](tests/) using `import stable_retro as retro` pattern:

- [`tests/test_python/test_roms.py`](tests/test_python/test_roms.py:4-5): `import retro.data` → `import stable_retro.data`, etc.
- [`tests/test_python/test_load.py`](tests/test_python/test_load.py:10): `import retro` → `import stable_retro as retro`
- [`tests/test_python/test_integration.py`](tests/test_python/test_integration.py:4): Update imports appropriately
- [`tests/test_python/test_paths.py`](tests/test_python/test_paths.py:5): `import retro` → `import stable_retro as retro`
- [`tests/test_python/test_env.py`](tests/test_python/test_env.py:5,15): `import retro` → `import stable_retro as retro`, `import stable_retro.data`
- [`tests/test_python/__init__.py`](tests/test_python/__init__.py:5-6): Update imports appropriately

**Note:** Using the alias pattern in tests minimizes code changes while ensuring tests validate the new package name.

### Phase 7: Update C++ Source Files (if needed)

Check C++ source files in [`src/`](src/) for any hardcoded "retro" strings that might affect:
- Module names
- Error messages
- Documentation strings

Files to review:
- All `.cpp` and `.h` files for string literals containing "retro"

### Phase 8: Verification Steps

After making all changes:

1. **Build the package**:
   ```bash
   python setup.py build_ext --inplace
   ```

2. **Test new import (with alias)**:
   ```python
   import stable_retro as retro
   env = retro.make('Airstriker-Genesis-v0')
   ```

3. **Test new import (without alias)**:
   ```python
   import stable_retro
   env = stable_retro.make('Airstriker-Genesis-v0')
   ```

4. **Test backward compatibility**:
   ```python
   import retro  # Should show deprecation warning
   env = retro.make('Airstriker-Genesis-v0')
   ```

4. **Run test suite**:
   ```bash
   pytest tests/
   ```

5. **Test all examples**:
   ```bash
   python -m stable_retro.examples.random_agent --game Airstriker-Genesis-v0
   ```

6. **Build documentation**:
   ```bash
   cd docs
   make html
   ```

7. **Test package installation**:
   ```bash
   pip install -e .
   python -c "import stable_retro; print(stable_retro.__version__)"
   python -c "import retro; print(retro.__version__)"  # Should warn
   ```

### Phase 9: Update Release Documentation

Update the following files:
- [`CHANGES.md`](CHANGES.md): Add entry about the package rename
- [`README.md`](README.md): Update any code examples to use `stable_retro`
- [`CONTRIBUTING.md`](CONTRIBUTING.md): Update contribution guidelines if they reference imports

Example CHANGES.md entry:
```markdown
## Version X.X.X

### Breaking Changes (with backward compatibility)
- **Package import name changed from `retro` to `stable_retro`**
  - Users should now use `import stable_retro` instead of `import retro`
  - The old `import retro` will continue to work with a deprecation warning
  - This aligns the Python import name with the PyPI package name `stable-retro`
  - All internal code has been updated to use `stable_retro`
  - Documentation updated to reflect new import name
  - Backward compatibility will be maintained for at least 2 major versions
```

## Migration Timeline

1. **Version X.X.X (current)**: Implement rename with backward compatibility
2. **Version X+1.X.X**: Continue showing deprecation warnings
3. **Version X+2.X.X**: Increase warning severity (FutureWarning)
4. **Version X+3.X.X**: Remove backward compatibility layer

## Risks and Mitigation

### Risk 1: Breaking existing user code
**Mitigation**: Backward compatibility layer with clear deprecation warnings

### Risk 2: Third-party packages depending on `retro`
**Mitigation**: Maintain compatibility layer for multiple versions, communicate change clearly

### Risk 3: Documentation and examples scattered across the internet
**Mitigation**: Both import methods work, comprehensive migration guide in docs

### Risk 4: Build system issues with renamed C extensions
**Mitigation**: Thorough testing of build process on all platforms (Linux, macOS, Windows)

## Success Criteria

- ✅ `import stable_retro` works correctly
- ✅ `import stable_retro as retro` works correctly (minimal migration path)
- ✅ `import retro` works with deprecation warning (backward compatibility)
- ✅ All tests pass with new import name
- ✅ All examples run with new import name
- ✅ Documentation builds successfully
- ✅ Package installs correctly via pip
- ✅ C extensions compile and load properly
- ✅ Backward compatibility verified

## Notes

- The PyPI package name `stable-retro` (with hyphen) is already correct and doesn't need to change
- Only the Python import name is changing from `retro` to `stable_retro` (with underscore)
- This change aligns the import name with the package name for better consistency
- The C extension module `_retro` becomes `stable_retro._retro`
- **Using `import stable_retro as retro` provides the easiest migration path** - code continues to reference `retro.*` while using the new package name
- Internal files use the alias pattern to minimize changes while ensuring correct package references
