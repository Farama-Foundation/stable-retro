"""
Backward compatibility module for retro -> stable_retro migration.

This module provides a compatibility layer that allows 'import retro' to continue
working while warning users about the deprecation.
"""

import importlib
import sys
import warnings

import stable_retro as _stable_retro
import stable_retro._retro
import stable_retro.data
import stable_retro.enums
import stable_retro.examples
import stable_retro.retro_env
import stable_retro.scripts
import stable_retro.testing

# Import and re-export everything from stable_retro
from stable_retro import *  # noqa: F401, F403

# Issue deprecation warning (after imports to satisfy E402)
warnings.warn(
    "The 'retro' package name is deprecated and will be removed in a future version. "
    "Please use 'import stable_retro' instead of 'import retro'.",
    DeprecationWarning,
    stacklevel=2,
)

stable_retro_import = importlib.import_module("stable_retro.import")

# Map the modules
sys.modules["retro"] = sys.modules["stable_retro"]
sys.modules["retro.data"] = stable_retro.data
sys.modules["retro.scripts"] = stable_retro.scripts
sys.modules["retro.examples"] = stable_retro.examples
sys.modules["retro.import"] = stable_retro_import
sys.modules["retro.testing"] = stable_retro.testing
sys.modules["retro._retro"] = stable_retro._retro
sys.modules["retro.enums"] = stable_retro.enums
sys.modules["retro.retro_env"] = stable_retro.retro_env

# Try to import rendering if it exists
try:
    import stable_retro.rendering

    sys.modules["retro.rendering"] = stable_retro.rendering
except (ImportError, AttributeError):
    pass

# Re-export commonly used items
__version__ = _stable_retro.__version__
__all__ = _stable_retro.__all__
