import os
import pickle

import pytest

import stable_retro as retro


def test_env_create(generate_test_env):
    json_path = os.path.join(os.path.dirname(__file__), "../dummy.json")
    assert generate_test_env(info=json_path, scenario=json_path)


@pytest.mark.parametrize("obs_type", [retro.Observations.IMAGE, retro.Observations.RAM])
def test_env_basic(obs_type, generate_test_env):
    json_path = os.path.join(os.path.dirname(__file__), "../dummy.json")

    env = generate_test_env(info=json_path, scenario=json_path, obs_type=obs_type)

    obs, info = env.reset()
    assert obs in env.observation_space
    assert isinstance(info, dict)
    assert env.viewer is None

    obs, rew, terminated, truncated, info = env.step(env.action_space.sample())
    assert obs in env.observation_space
    assert env.viewer is None

    assert isinstance(rew, float)
    assert rew == 0

    assert isinstance(terminated, bool)
    assert isinstance(truncated, bool)

    assert terminated is False
    assert truncated is False

    assert isinstance(info, dict)


def test_env_data(generate_test_env):
    json_path = os.path.join(os.path.dirname(__file__), "../dummy.json")

    env = generate_test_env(info=json_path, scenario=json_path)
    assert isinstance(env.data[env.system], int)

    env.data["foo"] = 1
    assert env.data["foo"] == 1

    env.reset()

    with pytest.raises(KeyError):
        val = env.data["foo"]
        assert val


def test_env_pickle_roundtrip(generate_test_env):
    json_path = os.path.join(os.path.dirname(__file__), "../dummy.json")

    env = generate_test_env(
        info=json_path,
        scenario=json_path,
        render_mode="rgb_array",
    )
    env.reset()

    payload = pickle.dumps(env)
    env.close()

    restored = pickle.loads(payload)
    try:
        obs, info = restored.reset()
        assert obs in restored.observation_space
        assert isinstance(info, dict)

        obs, rew, terminated, truncated, info = restored.step(
            restored.action_space.sample(),
        )
        assert obs in restored.observation_space
        assert isinstance(rew, float)
        assert isinstance(terminated, bool)
        assert isinstance(truncated, bool)
        assert isinstance(info, dict)
    finally:
        restored.close()


def test_env_pickle_roundtrip_does_not_serialize_open_movie(
    generate_test_env,
    tmp_path,
):
    json_path = os.path.join(os.path.dirname(__file__), "../dummy.json")

    env = generate_test_env(
        info=json_path,
        scenario=json_path,
        record=str(tmp_path),
        render_mode="rgb_array",
    )
    if env.statename is None:
        env.close()
        pytest.skip("recording path requires a named state in this fixture setup")

    env.reset()
    assert env.movie is not None

    payload = pickle.dumps(env)
    env.close()

    restored = pickle.loads(payload)
    try:
        assert restored.movie is None
        assert restored.movie_path == str(tmp_path)

        restored.reset()
        assert restored.movie is not None
    finally:
        restored.close()


def test_env_state_replay(generate_test_env):
    """Core state restore must reproduce subsequent RGB frames on every host."""
    import numpy as np

    json_path = os.path.join(os.path.dirname(__file__), "../dummy.json")
    env = generate_test_env(info=json_path, scenario=json_path)
    env.reset()
    action = np.zeros(env.action_space.shape, dtype=np.int8)
    for _ in range(120):
        env.step(action)
    state = env.em.get_state()
    assert state
    expected = [env.step(action)[0].copy() for _ in range(20)]
    env.em.set_state(state)
    for image in expected:
        np.testing.assert_array_equal(env.step(action)[0], image)
