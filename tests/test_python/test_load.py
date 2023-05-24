import gc
import gzip
import zlib
from concurrent.futures import ProcessPoolExecutor, TimeoutError
from concurrent.futures.process import BrokenProcessPool

import pytest

import retro
from tests.test_python import all_games

pool = ProcessPoolExecutor(1)


@pytest.fixture(scope="module")
def processpool():
    def run(fn, *args):
        global pool
        try:
            future = pool.submit(fn, *args)
            return future.result(2)
        except BrokenProcessPool:
            pool = ProcessPoolExecutor(1)
            return [], [(args[0], "subprocess crashed")]
        except TimeoutError:
            return [], [(args[0], "task timed out")]

    yield run
    pool.shutdown()


def load(game, inttype):
    errors = []
    rom = retro.data.get_romfile_path(game, inttype)
    emu = retro.RetroEmulator(rom)

    emu.step()

    del emu
    gc.collect()

    return [], errors


def state(game, inttype):
    errors = []
    states = retro.data.list_states(game, inttype)
    if not states:
        return [], []

    rom = retro.data.get_romfile_path(game, inttype | retro.data.Integrations.STABLE)
    emu = retro.RetroEmulator(rom)
    for state_file in states:
        try:
            with gzip.open(
                retro.data.get_file_path(game, state_file + ".state", inttype), "rb"
            ) as fh:
                game_state = fh.read()
        except (OSError, zlib.error):
            errors.append((game, f"state failed to decode: {state_file}"))
            continue

        emu.set_state(game_state)
        emu.step()

    del emu
    gc.collect()

    return [], errors


@pytest.mark.parametrize("game_name, integration_type", all_games)
def test_load(game_name, integration_type, processpool):
    warnings, errors = processpool(load, game_name, integration_type)
    assert len(warnings) == 0
    assert len(errors) == 0


@pytest.mark.parametrize("game_name, integration_type", all_games)
def test_state(game_name, integration_type, processpool):
    warnings, errors = processpool(state, game_name, integration_type)
    assert len(warnings) == 0
    assert len(errors) == 0
