import argparse

import stable_retro as retro
from stable_retro.examples.interactive import Interactive


class RetroInteractive(Interactive):
    """
    Interactive setup for retro games
    """

    def __init__(self, game, state, scenario):
        env = retro.make(
            game=game,
            state=state,
            scenario=scenario,
            render_mode="rgb_array",
        )
        self._buttons = env.buttons
        super().__init__(env=env, sync=False, tps=60, aspect_ratio=4 / 3)

    def get_image(self, _obs, env):
        return env.render()

    def keys_to_act(self, keys):
        inputs = {
            None: False,
            "BUTTON": "Z" in keys,
            "A": "Z" in keys,
            "B": "X" in keys,
            "C": "C" in keys,
            "X": "A" in keys,
            "Y": "S" in keys,
            "Z": "D" in keys,
            "L": "Q" in keys,
            "R": "W" in keys,
            "UP": "UP" in keys,
            "DOWN": "DOWN" in keys,
            "LEFT": "LEFT" in keys,
            "RIGHT": "RIGHT" in keys,
            "MODE": "TAB" in keys,
            "SELECT": "TAB" in keys,
            "RESET": "ENTER" in keys,
            "START": "ENTER" in keys,
        }
        return [inputs[b] for b in self._buttons]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game", default="SonicTheHedgehog-Genesis")
    parser.add_argument("--state", default=retro.State.DEFAULT)
    parser.add_argument("--scenario", default="scenario")
    args = parser.parse_args()

    ia = RetroInteractive(game=args.game, state=args.state, scenario=args.scenario)
    ia.run()


if __name__ == "__main__":
    main()
