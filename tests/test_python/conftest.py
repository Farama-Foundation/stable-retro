from pathlib import Path

import pytest

import stable_retro as retro

ROM_DIR = Path(__file__).resolve().parent.parent / "roms"


@pytest.fixture(params=sorted(rom.stem for rom in ROM_DIR.iterdir()))
def generate_test_env(request, monkeypatch):
    """Exercise each redistributable ROM through the normal environment API."""
    monkeypatch.setattr(
        retro.data,
        "get_file_path",
        lambda game, file, *args, **kwargs: str(ROM_DIR / file),
    )
    monkeypatch.setattr(
        retro.data,
        "get_romfile_path",
        lambda game, *args, **kwargs: str(next(ROM_DIR.glob(game + ".*"))),
    )
    created_env = []

    def create(state=retro.State.NONE, *args, **kwargs):
        kwargs.setdefault("render_mode", "rgb_array")
        env = retro.make(game=request.param, state=state, *args, **kwargs)
        created_env.append(env)
        return env

    try:
        yield create
    finally:
        for env in reversed(created_env):
            env.close()
