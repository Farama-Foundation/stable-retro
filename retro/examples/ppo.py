"""
Train an agent using Proximal Policy Optimization from Stable Baselines 3
"""

import argparse

import numpy as np
import gymnasium as gym
from gymnasium.wrappers.time_limit import TimeLimit
from stable_baselines3.common.vec_env import SubprocVecEnv, VecFrameStack, VecTransposeImage
from stable_baselines3.common.atari_wrappers import WarpFrame, ClipRewardEnv 
from stable_baselines3 import PPO

import retro


class StochasticFrameSkip(gym.Wrapper):
    def __init__(self, env, n, stickprob):
        gym.Wrapper.__init__(self, env)
        self.n = n
        self.stickprob = stickprob
        self.curac = None
        self.rng = np.random.RandomState()
        self.supports_want_render = hasattr(env, "supports_want_render")

    def reset(self, **kwargs):
        self.curac = None
        return self.env.reset(**kwargs)

    def step(self, ac):
        terminated = False
        truncated = False
        totrew = 0
        for i in range(self.n):
            # First step after reset, use action
            if self.curac is None:
                self.curac = ac
            # First substep, delay with probability=stickprob
            elif i==0:
                if self.rng.rand() > self.stickprob:
                    self.curac = ac
            # Second substep, new action definitely kicks in
            elif i==1:
                self.curac = ac
            if self.supports_want_render and i<self.n-1:
                ob, rew, terminated, truncated, info = self.env.step(self.curac, want_render=False)
            else:
                ob, rew, terminated, truncated, info = self.env.step(self.curac)
            totrew += rew
            if terminated or truncated: break
        return ob, totrew, terminated, truncated, info


class ScaledFloatFrame(gym.ObservationWrapper):
    def __init__(self, env):
        gym.ObservationWrapper.__init__(self, env)
        self.observation_space = gym.spaces.Box(low=0, high=1, shape=env.observation_space.shape, dtype=np.float32)

    def observation(self, observation):
        # careful! This undoes the memory optimization, use
        # with smaller replay buffers only.
        return np.array(observation).astype(np.float32) / 255.0


def make_retro(*, game, state=None, max_episode_steps=4500, **kwargs):
    if state is None:
        state = retro.State.DEFAULT
    env = retro.make(game, state, **kwargs)
    env = StochasticFrameSkip(env, n=4, stickprob=0.25)
    if max_episode_steps is not None:
        env = TimeLimit(env, max_episode_steps=max_episode_steps)
    return env


def wrap_deepmind_retro(env, scale=True):
    """
    Configure environment for retro games, using config similar to DeepMind-style Atari in openai/baseline's wrap_deepmind
    """
    env = WarpFrame(env)
    env = ClipRewardEnv(env)
    # if scale:
    #     env = ScaledFloatFrame(env)
    return env


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--game', default='Airstriker-Genesis')
    parser.add_argument('--state', default=retro.State.DEFAULT)
    parser.add_argument('--scenario', default=None)
    args = parser.parse_args()

    def make_env():
        env = make_retro(game=args.game, state=args.state, scenario=args.scenario)
        env = wrap_deepmind_retro(env)
        return env

    venv = VecTransposeImage(VecFrameStack(SubprocVecEnv([make_env] * 8), n_stack=4))
    model = PPO(
        policy='CnnPolicy',
        env=venv,
        learning_rate=lambda f : f * 2.5e-4,
        n_steps=128,
        batch_size=32,
        n_epochs=4,
        gamma=0.99,
        gae_lambda=0.95,
        clip_range=0.1,
        ent_coef=0.01,
        verbose=1,
    )
    model.learn(
        total_timesteps=100_000_000,
        log_interval=1,
    )


if __name__ == '__main__':
    main()
