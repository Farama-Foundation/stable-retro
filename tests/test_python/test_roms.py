import gymnasium as gym
import pytest

import retro.data
import retro.testing.tools
from tests.test_python import all_games_with_roms


@pytest.mark.parametrize("game_name, integration_type", all_games_with_roms)
def test_hash(game_name, integration_type):
    errors = retro.data.verify_hash(game_name, integration_type)
    assert len(errors) == 0


def test_hash_collisions():
    warnings, errors = retro.testing.tools.verify_hash_collisions()
    for file, warning in warnings:
        gym.logger.warn(f"{file}: {warning}")
    assert len(errors) == 0


@pytest.mark.parametrize("game_name, integration_type", all_games_with_roms)
def test_rom(game_name, integration_type):
    warnings, errors = retro.testing.tools.verify_rom(game_name, integration_type)
    for file, warning in warnings:
        gym.logger.warn(f"{file}: {warning}")
    assert len(errors) == 0
