"""Run original DS machine code without requiring proprietary BIOS or ROM files."""

import os
import struct

import numpy as np
import pytest

import stable_retro as retro


@pytest.fixture
def ds_emulator(tmp_path):
    if "NintendoDs" not in retro.data.EMU_CORES:
        pytest.skip("Nintendo DS core is not installed")

    # Original ARM9 program: power on the main display on the bottom panel,
    # map VRAM A to LCD memory, select framebuffer display mode, and fill
    # the upper/lower halves with distinct colors. ARM7 simply loops.
    # Literal loads are PC-relative; the last five words are their constants.
    arm9 = (
        0xE59F004C,  # ldr r0, =0x04000304 (POWCNT1)
        0xE59F104C,  # ldr r1, =0x00000203
        0xE1C010B0,  # strh r1, [r0]
        0xE59F0048,  # ldr r0, =0x04000240 (VRAMCNT_A)
        0xE3A01080,  # mov r1, #0x80
        0xE5C01000,  # strb r1, [r0]
        0xE3A00301,  # mov r0, #0x04000000 (DISPCNT)
        0xE3A01802,  # mov r1, #0x20000
        0xE5801000,  # str r1, [r0]
        0xE3A0051A,  # mov r0, #0x06800000 (LCD VRAM A)
        0xE59F2030,  # ldr r2, =0x0680c000
        0xE3A0101F,  # mov r1, #0x1f
        0xE0C010B2,  # strh r1, [r0], #2
        0xE1500002,  # cmp r0, r2
        0x3AFFFFFC,  # blo to first strh
        0xE59F2020,  # ldr r2, =0x06818000
        0xE3A01E3E,  # mov r1, #0x3e0
        0xE0C010B2,  # strh r1, [r0], #2
        0xE1500002,  # cmp r0, r2
        0x3AFFFFFC,  # blo to second strh
        0xEAFFFFFE,  # b .
        0x04000304,
        0x00000203,
        0x04000240,
        0x0680C000,
        0x06818000,
    )
    code = struct.pack(f"<{len(arm9)}I", *arm9)
    rom = bytearray(0x20000)
    rom[:12] = b"ARM64 PROBE "
    rom[12:16] = b"####"  # Homebrew identifier, not a retail game code.
    struct.pack_into("<4I", rom, 0x20, 0x200, 0x02000000, 0x02000000, len(code))
    struct.pack_into("<4I", rom, 0x30, 0x400, 0x03800000, 0x03800000, 4)
    struct.pack_into("<2I", rom, 0x80, len(rom), 0x200)
    rom[0x200 : 0x200 + len(code)] = code
    struct.pack_into("<I", rom, 0x400, 0xEAFFFFFE)
    path = tmp_path / "display.nds"
    path.write_bytes(rom)
    emulator = retro.RetroEmulator(str(path))
    for _ in range(120):
        emulator.step()
    yield emulator
    del emulator


def test_ds_display_and_state_replay(ds_emulator):
    image = ds_emulator.get_screen().copy()
    assert image.shape == (192, 256, 3)
    assert np.all(image[:96] == image[0, 0])
    assert np.all(image[96:] == image[-1, 0])
    assert not np.array_equal(image[0, 0], image[-1, 0])
    state = ds_emulator.get_state()
    assert state
    expected = []
    for _ in range(20):
        ds_emulator.step()
        expected.append(ds_emulator.get_screen().copy())
    ds_emulator.set_state(state)
    for frame in expected:
        ds_emulator.step()
        np.testing.assert_array_equal(ds_emulator.get_screen(), frame)


@pytest.mark.skipif(
    os.environ.get("STABLE_RETRO_TEST_RENDERING") != "1",
    reason="requires an OpenGL display (STABLE_RETRO_TEST_RENDERING=1)",
)
def test_ds_window_rendering(ds_emulator):
    import pyglet

    from stable_retro.rendering import ImageRenderer

    from .test_rendering import assert_corners

    window = pyglet.window.Window(width=256, height=192, visible=False)
    renderer = ImageRenderer(window)
    try:
        image = ds_emulator.get_screen()
        renderer.draw(image)
        assert_corners(window, image)
    finally:
        renderer.close()
        window.close()
