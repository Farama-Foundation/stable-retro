"""Compatibility shim for ``retro.rendering``.

Importing this module forwards to ``stable_retro.rendering`` without forcing
the renderer to initialize during ``import retro``.
"""

from stable_retro.rendering import *  # noqa: F401, F403
