"""Every installed core must expose a usable native libretro interface."""

import ctypes

import pytest

import stable_retro as retro


class SystemInfo(ctypes.Structure):
    _fields_ = [
        ("library_name", ctypes.c_char_p),
        ("library_version", ctypes.c_char_p),
        ("valid_extensions", ctypes.c_char_p),
        ("need_fullpath", ctypes.c_bool),
        ("block_extract", ctypes.c_bool),
    ]


@pytest.mark.parametrize("system", sorted(retro.data.EMU_CORES))
def test_core_native_interface(system):
    # Includes cores without redistributable game fixtures, such as Saturn/DS.
    core = ctypes.CDLL(retro.get_core_path(system))
    core.retro_api_version.argtypes = []
    core.retro_api_version.restype = ctypes.c_uint
    assert core.retro_api_version() == 1
    core.retro_get_system_info.argtypes = [ctypes.POINTER(SystemInfo)]
    core.retro_get_system_info.restype = None
    info = SystemInfo()
    core.retro_get_system_info(ctypes.byref(info))
    assert info.library_name
    assert info.library_version
    assert info.valid_extensions


def test_snes_imported_save_state():
    """Optional real-game regression for the ARM64 byte-order bug."""
    import hashlib

    game = "SuperMarioWorld-Snes-v0"
    try:
        retro.data.get_romfile_path(game)
    except FileNotFoundError:
        pytest.skip(f"{game} ROM is not installed")

    with retro.make(game, state="Start", render_mode="rgb_array") as env:
        image, _ = env.reset()
        assert image.shape == (224, 256, 3)
        # Reference verified on x86-64 Linux and corrected ARM64 Snes9x. The
        # broken core returns a valid-shaped image with corrupt sprites/HUD.
        assert hashlib.sha256(image.tobytes()).hexdigest() == (
            "be19b96ced72a2e0460effa5ada2141c2e57bd565bd9f289ab312cd5553870e8"
        )
