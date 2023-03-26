import retro


def main():
    env = retro.make(game="Airstriker-Genesis")
    env.reset()
    while True:
        obs, rew, done, info = env.step(env.action_space.sample())
        env.render()
        if done:
            env.reset()
    env.close()


if __name__ == "__main__":
    main()
