"""
Interact with Gym environments using the keyboard

An adapter object is defined for each environment to map keyboard commands to actions and extract observations as pixels.
"""

import abc
import argparse
import time

import numpy as np
import pyglet
from pyglet.window import key as keycodes

import stable_retro as retro
from stable_retro.rendering import ImageRenderer, get_window


class Interactive(abc.ABC):
    """
    Base class for making gym environments interactive for human use
    """

    def __init__(self, env, sync=True, tps=60, aspect_ratio=None):
        obs = env.reset()
        self._image = self.get_image(obs, env)
        assert (
            len(self._image.shape) == 3 and self._image.shape[2] == 3
        ), "must be an RGB image"
        image_height, image_width = self._image.shape[:2]

        if aspect_ratio is None:
            aspect_ratio = image_width / image_height

        # guess a screen size that doesn't distort the image too much but also is not tiny or huge
        display = pyglet.display.get_display()
        screen = display.get_default_screen()
        max_win_width = screen.width * 0.9
        max_win_height = screen.height * 0.9
        win_width = image_width
        win_height = int(win_width / aspect_ratio)

        while win_width > max_win_width or win_height > max_win_height:
            win_width //= 2
            win_height //= 2
        while win_width < max_win_width / 2 and win_height < max_win_height / 2:
            win_width *= 2
            win_height *= 2

        win = get_window(width=win_width, height=win_height, display=display)

        self._key_handler = pyglet.window.key.KeyStateHandler()
        win.push_handlers(self._key_handler)
        win.on_close = self._on_close

        self._renderer = ImageRenderer(win)
        self._closed = False

        self._env = env
        self._win = win

        # self._render_human = render_human
        self._key_previous_states = set()

        self._steps = 0
        self._episode_steps = 0
        self._episode_returns = 0
        self._prev_episode_returns = 0

        self._tps = tps
        self._sync = sync
        self._current_time = 0
        self._sim_time = 0
        self._max_sim_frames_per_update = 4

    def _update(self, dt):
        # cap the number of frames rendered so we don't just spend forever trying to catch up on frames
        # if rendering is slow
        max_dt = self._max_sim_frames_per_update / self._tps
        if dt > max_dt:
            dt = max_dt

        # catch up the simulation to the current time
        self._current_time += dt
        while self._sim_time < self._current_time:
            self._sim_time += 1 / self._tps

            keys_pressed = {
                code for code, pressed in self._key_handler.data.items() if pressed
            }
            keys_clicked = keys_pressed - self._key_previous_states
            self._key_previous_states = keys_pressed

            if keycodes.ESCAPE in keys_pressed:
                self._on_close()
                return

            # assume that for async environments, we just want to repeat keys for as long as they are held
            inputs = keys_pressed
            if self._sync:
                inputs = keys_clicked

            keys = []
            for keycode in inputs:
                for name in dir(keycodes):
                    if getattr(keycodes, name) == keycode:
                        keys.append(name)

            act = self.keys_to_act(keys)

            if not self._sync or act is not None:
                obs, rew, terminated, truncated, _info = self._env.step(act)
                done = terminated or truncated
                self._image = self.get_image(obs, self._env)
                self._episode_returns += rew
                self._steps += 1
                self._episode_steps += 1
                np.set_printoptions(precision=2)
                if self._sync:
                    done_int = int(done)  # shorter than printing True/False
                    mess = f"steps={self._steps} episode_steps={self._episode_steps} rew={rew} episode_returns={self._episode_returns} done={done_int}"
                    print(mess)
                elif self._steps % self._tps == 0 or done:
                    episode_returns_delta = (
                        self._episode_returns - self._prev_episode_returns
                    )
                    self._prev_episode_returns = self._episode_returns
                    mess = f"steps={self._steps} episode_steps={self._episode_steps} episode_returns_delta={episode_returns_delta} episode_returns={self._episode_returns}"
                    print(mess)

                if done:
                    self._env.reset()
                    self._episode_steps = 0
                    self._episode_returns = 0
                    self._prev_episode_returns = 0

    def _draw(self):
        self._renderer.draw(self._image)

    def _on_close(self):
        if not self._closed:
            self._closed = True
            self._renderer.close()
            self._win.close()
            self._env.close()
        return pyglet.event.EVENT_HANDLED

    @abc.abstractmethod
    def get_image(self, obs, venv):
        """
        Given an observation and the Env object, return an rgb array to display to the user
        """
        pass

    @abc.abstractmethod
    def keys_to_act(self, keys):
        """
        Given a list of keys that the user has input, produce a gym action to pass to the environment

        For sync environments, keys is a list of keys that have been pressed since the last step
        For async environments, keys is a list of keys currently held down
        """
        pass

    def run(self):
        """
        Run the interactive window until the user quits
        """
        prev_frame_time = time.perf_counter()
        try:
            while not self._closed:
                self._win.dispatch_events()
                if self._closed:
                    break
                now = time.perf_counter()
                self._update(now - prev_frame_time)
                prev_frame_time = now
                if self._closed:
                    break
                self._draw()
                self._win.flip()
                time.sleep(max(0, 1 / self._tps - (time.perf_counter() - now)))
        finally:
            self._on_close()


class RetroInteractive(Interactive):
    """
    Interactive setup for retro games
    """

    def __init__(self, game, state, scenario, record):
        env = retro.make(
            game=game,
            state=state,
            scenario=scenario,
            record=record,
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
    parser.add_argument("--game", default="Airstriker-Genesis")
    parser.add_argument("--state", default=retro.State.DEFAULT)
    parser.add_argument("--scenario", default=None)
    parser.add_argument("--record", default=None, nargs="?", const=True)
    args = parser.parse_args()

    ia = RetroInteractive(
        game=args.game,
        state=args.state,
        scenario=args.scenario,
        record=args.record,
    )
    ia.run()


if __name__ == "__main__":
    main()
