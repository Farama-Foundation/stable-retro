import hashlib
import json

import gymnasium as gym
import pytest

import stable_retro.data
import stable_retro.testing.tools
from tests.test_python import all_games_with_roms


def test_merge_nested_original_rom_name(tmp_path, monkeypatch):
    game = "Test-Quake-v0"
    game_path = tmp_path / game
    game_path.mkdir()
    (game_path / "metadata.json").write_text(
        json.dumps({"original_rom_name": "id1/pak0.pak"}),
    )
    rom_data = b"quake-pak"
    source = tmp_path / "source.pak"
    source.write_bytes(rom_data)
    rom_hash = hashlib.sha1(rom_data).hexdigest()
    monkeypatch.setattr(
        stable_retro.data,
        "get_known_hashes",
        lambda: {rom_hash: (game, ".pak", str(tmp_path))},
    )

    stable_retro.data.merge(str(source))

    assert (game_path / "rom.pak").read_bytes() == rom_data
    assert (game_path / "id1" / "pak0.pak").read_bytes() == rom_data


@pytest.mark.parametrize("game_name, integration_type", all_games_with_roms)
def test_hash(game_name, integration_type):
    errors = stable_retro.data.verify_hash(game_name, integration_type)
    assert len(errors) == 0


def test_hash_collisions():
    warnings, errors = stable_retro.testing.tools.verify_hash_collisions()
    for file, warning in warnings:
        gym.logger.warn(f"{file}: {warning}")
    assert len(errors) == 0


@pytest.mark.parametrize("game_name, integration_type", all_games_with_roms)
def test_rom(game_name, integration_type):
    warnings, errors = stable_retro.testing.tools.verify_rom(
        game_name,
        integration_type,
    )
    for file, warning in warnings:
        gym.logger.warn(f"{file}: {warning}")
    assert len(errors) == 0
