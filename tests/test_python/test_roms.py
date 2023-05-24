import pytest

import retro.data
import retro.testing.tools
from tests.test_python import all_games


@pytest.mark.parametrize("game_name, integration_type", all_games)
def test_hash(game_name, integration_type):
    errors = retro.data.verify_hash(game_name, integration_type)
    assert len(errors) == 0


def test_hash_collisions():
    warnings, errors = retro.testing.tools.verify_hash_collisions()
    assert len(warnings) == 0
    assert len(errors) == 0


@pytest.mark.parametrize("game_name, integration_type", all_games)
def test_rom(game_name, integration_type):
    warnings, errors = retro.testing.tools.verify_rom(game_name, integration_type)
    assert len(warnings) == 0
    assert len(errors) == 0
