"""Controller bounds use bundled homebrew ROMs; no commercial ROM is needed."""

import gc
import pickle
import zipfile
from pathlib import Path

import numpy as np
import pytest

import stable_retro as retro

ROM = Path(__file__).parents[1] / "roms" / "Dekadence-Dekadrive.md"


@pytest.fixture
def emulator():
    em = retro.RetroEmulator(str(ROM))
    yield em
    del em
    gc.collect()


@pytest.fixture
def make_env(monkeypatch):
    dummy = str(Path(__file__).parents[1] / "dummy.json")
    monkeypatch.setattr(retro.data, "get_original_romfile_path", lambda *args: str(ROM))

    def create(**kwargs):
        return retro.RetroEnv(
            "homebrew",
            state=retro.State.NONE,
            info=dummy,
            scenario=dummy,
            render_mode="rgb_array",
            **kwargs,
        )

    return create


def test_controller_devices_and_bounds(emulator):
    assert emulator.max_players == 4
    assert emulator.controller_devices[0][1025] == "MD Joypad 3 Button + 4-WayPlay"
    emulator.set_controller_port_device(0, 1025)
    for port in range(4):
        emulator.set_button_mask(np.ones(16, dtype=np.uint8), port)
    for port in (-1, 4, 99):
        with pytest.raises((IndexError, TypeError, RuntimeError)):
            emulator.set_button_mask(np.zeros(12, dtype=np.uint8), port)
        with pytest.raises((IndexError, TypeError)):
            emulator.set_controller_port_device(port, 1025)
    with pytest.raises(IndexError):
        emulator.set_controller_port_device(2, 1025)
    with pytest.raises(ValueError):
        emulator.set_controller_port_device(0, 9999)
    with pytest.raises(RuntimeError):
        emulator.set_button_mask(np.zeros(17, dtype=np.uint8))
    with pytest.raises(ValueError):
        emulator.set_button_mask(np.zeros((2, 6), dtype=np.uint8))
    with pytest.raises(ValueError):
        emulator.set_button_mask(np.zeros(16, dtype=np.uint8)[::2])
    snapshot = emulator.get_state()
    for _ in range(4):
        assert emulator.set_state(snapshot)
        emulator.step()
    assert emulator.controller_devices[0][1025]


@pytest.mark.parametrize("players", [0, 5, 99])
def test_movie_rejects_invalid_counts(tmp_path, players):
    with pytest.raises(ValueError):
        retro.Movie(str(tmp_path / "invalid.bk2"), True, players)


def test_movie_indices_and_four_player_roundtrip(tmp_path, emulator):
    path = str(tmp_path / "four.bk2")
    movie = retro.Movie(path, True, 4)
    movie.configure("homebrew", emulator)
    movie.set_state(emulator.get_state())
    for key, player in ((0, 0), (15, 3)):
        movie.set_key(key, True, player)
        assert movie.get_key(key, player)
    for key, player in ((-1, 0), (16, 0), (0, 4), (0, 99)):
        with pytest.raises(IndexError):
            movie.get_key(key, player)
        with pytest.raises(IndexError):
            movie.set_key(key, True, player)
    # Genesis records twelve buttons; first and last controller both survive.
    movie.set_key(7, True, 3)
    movie.step()
    movie.close()
    replay = retro.Movie(path)
    assert replay.players == 4
    assert replay.get_state() == emulator.get_state()
    assert replay.step()
    assert replay.get_key(0, 0) and replay.get_key(7, 3)
    replay.close()
    narrow = retro.Movie(str(tmp_path / "one.bk2"), True, 1)
    with pytest.raises(IndexError):
        narrow.get_key(0, 1)
    narrow.close()


@pytest.mark.parametrize("player", [0, 5, 999999999999999999999999999])
def test_movie_import_rejects_invalid_players(tmp_path, player):
    path = tmp_path / "bad.bk2"
    with zipfile.ZipFile(path, "w") as archive:
        archive.writestr("Header.txt", "Platform GEN\nGameName homebrew\n")
        archive.writestr("Core.bin", b"")
        archive.writestr(
            "Input Log.txt",
            f"[Input]\nLogKey:#Reset|Power|#P{player} B|\n|..|B|\n",
        )
    with pytest.raises((ValueError, IndexError)):
        retro.Movie(str(path))


@pytest.mark.parametrize("players", [-1, 0, 5, True, 1.5, None])
def test_env_rejects_invalid_players_before_rom_lookup(players):
    with pytest.raises(ValueError, match="players"):
        retro.RetroEnv("does-not-exist", players=players)


@pytest.mark.parametrize("players", [1, 2, 3, 4])
def test_env_multibinary_scalar_reward_and_reset(make_env, players):
    env = make_env(
        players=players,
        controller_devices={0: 1025} if players > 2 else None,
    )
    try:
        observation, info = env.reset()
        assert observation in env.observation_space and isinstance(info, dict)
        assert env.action_space.shape == (12 * players,)
        assert env.multi_rewards is False
        for _ in range(2):
            observation, reward, terminated, truncated, info = env.step(
                np.zeros(12 * players, dtype=np.int8),
            )
            assert observation in env.observation_space
            assert isinstance(reward, float)
            assert isinstance(terminated, bool) and isinstance(truncated, bool)
            env.reset()
    finally:
        env.close()


@pytest.mark.parametrize(
    "actions",
    [
        retro.Actions.ALL,
        retro.Actions.FILTERED,
        retro.Actions.DISCRETE,
        retro.Actions.MULTI_DISCRETE,
    ],
)
def test_four_player_action_spaces_and_rewards(make_env, actions):
    env = make_env(
        players=4,
        controller_devices={0: 1025},
        use_restricted_actions=actions,
    )
    try:
        env.reset()
        action = env.action_space.sample()
        assert len(env.action_to_array(action)) == 4
        observation, reward, terminated, truncated, info = env.step(action)
        assert observation in env.observation_space
        assert isinstance(reward, float)
        if actions == retro.Actions.DISCRETE:
            assert env.action_space.n == (
                np.prod([len(group) for group in env.button_combos]) ** 4
            )
        elif actions == retro.Actions.MULTI_DISCRETE:
            assert (
                env.action_space.nvec.tolist()
                == [len(group) for group in env.button_combos] * 4
            )
        else:
            assert env.action_space.shape == (env.num_buttons * 4,)
        env.multi_rewards = True
        _, reward, _, _, _ = env.step(action)
        assert isinstance(reward, list) and len(reward) == 4
        assert all(isinstance(value, float) for value in reward)
    finally:
        env.close()


def test_controller_devices_pickle_roundtrip(make_env):
    env = make_env(players=4, controller_devices={0: 1025})
    try:
        env.reset()
        serialized = pickle.dumps(env)
    finally:
        env.close()
    restored = pickle.loads(serialized)
    try:
        assert restored.players == 4
        assert restored._ezpickle_kwargs == env._ezpickle_kwargs
        assert restored._ezpickle_args[-1] == {0: 1025}
        observation, _ = restored.reset()
        assert observation in restored.observation_space
        _, reward, _, _, _ = restored.step(np.zeros(48, dtype=np.int8))
        assert isinstance(reward, float)
    finally:
        restored.close()


@pytest.mark.parametrize("players", [1, 2, 3, 4])
def test_multidiscrete_player_channels(make_env, players):
    env = make_env(
        players=players,
        controller_devices={0: 1025} if players > 2 else None,
        use_restricted_actions=retro.Actions.MULTI_DISCRETE,
    )
    try:
        num_combos = len(env.button_combos)
        action = np.zeros(num_combos * players, dtype=np.int64)
        for player in range(players):
            for group, combos in enumerate(env.button_combos):
                action[player * num_combos + group] = (player + group) % len(combos)
        masks = env.action_to_array(action)
        assert len(masks) == players
        for player, mask in enumerate(masks):
            expected = 0
            for group, combos in enumerate(env.button_combos):
                expected |= combos[action[player * num_combos + group]]
            assert mask.tolist() == [
                (expected >> button) & 1 for button in range(env.num_buttons)
            ]
    finally:
        env.close()


def test_invalid_device_releases_emulator(make_env):
    with pytest.raises(ValueError, match="not advertised"):
        make_env(players=4, controller_devices={0: 9999})
    env = make_env()
    try:
        observation, _ = env.reset()
        assert observation in env.observation_space
    finally:
        env.close()
