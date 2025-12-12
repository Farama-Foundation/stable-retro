from __future__ import annotations

import os

import stable_retro.data
from stable_retro.data import Integrations

all_games: list[tuple[str, Integrations]] = []
for game in stable_retro.data.list_games(Integrations.ALL):
    num_types = 0

    if os.path.exists(
        os.path.join(stable_retro.data.path(), *Integrations.STABLE.paths, game),
    ):
        all_games.append((game, Integrations.STABLE))
        num_types += 1

    if os.path.exists(
        os.path.join(
            stable_retro.data.path(),
            *Integrations.EXPERIMENTAL_ONLY.paths,
            game,
        ),
    ):
        all_games.append((game, Integrations.EXPERIMENTAL_ONLY))
        num_types += 1

    if os.path.exists(
        os.path.join(stable_retro.data.path(), *Integrations.CONTRIB_ONLY.paths, game),
    ):
        all_games.append((game, Integrations.CONTRIB_ONLY))
        num_types += 1

    if num_types > 1:
        all_games.append((game, Integrations.ALL))


all_games_with_roms: list[tuple[str, Integrations]] = []
for game, inttype in all_games:
    try:
        stable_retro.data.get_romfile_path(game, inttype)
        all_games_with_roms.append((game, inttype))
    except FileNotFoundError:
        pass
