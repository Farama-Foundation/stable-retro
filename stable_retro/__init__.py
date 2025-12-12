import os
import sys

import stable_retro.data
from stable_retro._retro import Movie, RetroEmulator, core_path
from stable_retro.enums import Actions, Observations, State
from stable_retro.retro_env import RetroEnv

ROOT_DIR = os.path.abspath(os.path.dirname(__file__))
core_path(os.path.join(os.path.dirname(__file__), "cores"))

with open(os.path.join(os.path.dirname(__file__), "VERSION.txt")) as f:
    __version__ = f.read()


__all__ = [
    "Movie",
    "RetroEmulator",
    "Actions",
    "State",
    "Observations",
    "get_core_path",
    "get_romfile_system",
    "get_system_info",
    "get_fbneo_rom_name",
    "is_fbneo_game",
    "make",
    "RetroEnv",
]

stable_retro.data.init_core_info(core_path())


def get_core_path(corename):
    return os.path.join(core_path(), stable_retro.data.EMU_CORES[corename])


def get_romfile_system(rom_path):
    extension = os.path.splitext(rom_path)[1]
    if extension in stable_retro.data.EMU_EXTENSIONS:
        return stable_retro.data.EMU_EXTENSIONS[extension]
    else:
        raise Exception(f"Unsupported rom type at path: {rom_path}")


def get_system_info(system):
    if system in stable_retro.data.EMU_INFO:
        return stable_retro.data.EMU_INFO[system]
    else:
        raise KeyError(f"Unsupported system type: {system}")


def make(
    game,
    state=State.DEFAULT,
    inttype=stable_retro.data.Integrations.DEFAULT,
    **kwargs,
):
    """
    Create a Gym environment for the specified game
    """
    try:
        stable_retro.data.get_romfile_path(game, inttype)
    except FileNotFoundError:
        if not stable_retro.data.get_file_path(game, "rom.sha", inttype):
            raise
        else:
            raise FileNotFoundError(
                f"Game not found: {game}. Did you make sure to import the ROM?",
            )
    return RetroEnv(game, state, inttype=inttype, **kwargs)


def get_fbneo_rom_name(game, inttype=stable_retro.data.Integrations.DEFAULT):
    """
    Get the original FBNeo ROM name for a game.
    Returns None if the game doesn't have an original_rom_name in metadata.

    Args:
        game: Game name
        inttype: Integration type (default: DEFAULT)

    Returns:
        str: Original ROM name (e.g., "mk2.zip") or None
    """
    import json

    metadata_path = stable_retro.data.get_file_path(game, "metadata.json", inttype)
    if metadata_path and os.path.exists(metadata_path):
        try:
            with open(metadata_path) as f:
                metadata = json.load(f)
                return metadata.get("original_rom_name")
        except (json.JSONDecodeError, OSError):
            pass
    return None


def is_fbneo_game(game, inttype=stable_retro.data.Integrations.DEFAULT):
    """
    Check if a game is an FBNeo game.

    Args:
        game: Game name
        inttype: Integration type (default: DEFAULT)

    Returns:
        bool: True if the game is an FBNeo game
    """
    import json

    metadata_path = stable_retro.data.get_file_path(game, "metadata.json", inttype)
    if metadata_path and os.path.exists(metadata_path):
        try:
            with open(metadata_path) as f:
                metadata = json.load(f)
                return metadata.get("system") == "FBNeo"
        except (json.JSONDecodeError, OSError):
            pass
    return False


try:
    from farama_notifications import notifications

    if "stable-retro" in notifications and __version__ in notifications["stable-retro"]:
        print(notifications["stable-retro"][__version__], file=sys.stderr)
except Exception:  # nosec
    pass
