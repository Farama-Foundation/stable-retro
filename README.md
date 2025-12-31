[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://pre-commit.com/) [![Code style: black](https://img.shields.io/badge/code%20style-black-000000.svg)](https://github.com/psf/black)

<p align="center">
    <a href="https://gymnasium.farama.org/" target = "_blank">
    <img src="docs/_static/img/stable-retro-text.png" width="500px" />
</a></p>

A fork of [gym-retro](https://github.com/openai/retro) ('lets you turn classic video games into Gymnasium environments for reinforcement learning') with additional games, emulators and supported platforms. Since gym-retro is in maintenance now, you can instead submit PRs with new games or features here in stable-retro.

- [Supported emulators](docs/supported_emulators.md)
- [Supported games/envs](docs/supported_games.md)

## Emulated Systems

| System| Linux | Windows | Apple |
| --- | --- | --- | --- |
| Atari 2600 | ✓ | ✓ | ✓ |
| NES | ✓ | ✓ | ✓ |
| SNES| ✓ | ✓ | ✓ |
| Nintendo 64 | ✓† | ✓† | — |
| Nintendo DS | ✓ | ✓ | ✓ |
| Gameboy/Color | ✓ | ✓ | ✓* |
| Gameboy Advance| ✓ | ✓ | ✓ |
| Sega Genesis | ✓ | ✓ | ✓ |
| Sega Master System | ✓ | ✓ | ✓ |
| Sega CD | ✓ | ✓ | ✓ |
| Sega 32X | ✓ | ✓ | ✓ |
| Sega Saturn | ✓ | ✓ | ✓ |
| Sega Dreamcast | ✓‡ | — | — |
| PC Engine | ✓ | ✓ | ✓ |
| Arcade Machines | ✓ | ✓ | — |

## Supported Games

Currently over 1000 games are integrated including:

| Category | Games |
| --- | --- |
| Platformers | Super Mario World, Sonic The Hedgehog 2, Mega Man 2, Castlevania IV |
| Fighters | Mortal Kombat Trilogy, Street Fighter II, Fatal Fury, King of Fighters '98 |
| Sports | NHL94, NBA Jam, Baseball Stars |
| Puzzle | Tetris, Columns |
| Shmups | 1943, Thunder Force IV, Gradius III, R-Type |
| BeatEmUps | Streets Of Rage, Double Dragon, TMNT 2: The Arcade Game, Golden Axe, Final Fight |
| Racing | Super Hang On, F-Zero, OutRun |
| RPGs (experimental) | Pokemon Red, Legend Of Zelda, Final Fantasy, Dragon Warrior |

> **Note:** If the game you want is not included but is supported by one of the systems in the list above, an integration tool is provided to help add new games.


## Installation

```
pip3 install stable-retro
```
or if the above doesn't work for your platform:
```
pip3 install git+https://github.com/Farama-Foundation/stable-retro.git
```

If you plan to integrate new ROMs, states or emulator cores or plan to edit an existing env:
```
git clone https://github.com/Farama-Foundation/stable-retro.git
cd stable-retro
pip3 install -e .
```

For platform-specific instructions including building from source, optional core dependencies, and the Integration UI:
- [Linux Installation](docs/linux_installation.md) - Ubuntu/Debian dependencies, N64 and Dreamcast core setup, WSL2 guide
- [macOS Installation](docs/macos_installation.md) - Apple Silicon build instructions, Homebrew dependencies

## Example

'Nature CNN' model trained using PPO on Airstriker-Genesis env (rom already included in the repo)

Tested on Ubuntu 20.04 and Windows 11 WSL2 (Ubuntu 20.04 VM)
```
sudo apt-get update
sudo apt-get install python3 python3-pip git zlib1g-dev libopenmpi-dev ffmpeg
```
You need to install a stable baselines 3 version that supports gymnasium
```
pip3 install git+https://github.com/Farama-Foundation/stable-retro.git
pip3 install stable_baselines3[extra]
```

Start training:
```
cd retro/examples
python3 ppo.py --game='Airstriker-Genesis-v0'
```

More advanced examples:
[https://github.com/MatPoliquin/stable-retro-scripts](https://github.com/MatPoliquin/stable-retro-scripts)

## Documentation & Tutorials

Documentation is available at [https://stable-retro.farama.org/](https://stable-retro.farama.org/) (work in progress)

See [LICENSES.md](https://github.com/Farama-Foundation/stable-retro/blob/master/LICENSES.md) for information on the licenses of the individual cores.

| Topic | Description |
| --- | --- |
| [Windows WSL2 Setup](https://www.youtube.com/watch?v=vPnJiUR21Og) | Step-by-step guide for setting up stable-retro on Windows 11 with WSL2 and Ubuntu 22.04 |
| [Game Integration Tool](https://www.youtube.com/playlist?list=PLmwlWbdWpZVvWqzOxu0jVBy-CaRpYha0t) | Playlist covering how to use the integration tool to add new games |
| [RetroArch + ML Models](https://www.youtube.com/watch?v=hkOcxJvJVjk) | Running a custom RetroArch build that supports overriding player input with trained models |

## ROMs and BIOS files

Each game integration has files listing memory locations for in-game variables, reward functions based on those variables, episode end conditions, savestates at the beginning of levels and a file containing hashes of ROMs that work with these files.

Please note that ROMs are not included and you must obtain them yourself.  Most ROM hashes are sourced from their respective No-Intro SHA-1 sums.

Run this script in the roms folder you want to import. If the checksum matches it will import them in the related game folder in stable-retro.
```bash
python3 -m retro.import .
```

Some platforms like Sega Saturn and Dreamcast also need to be provided a BIOS.
 [List of BIOS names and checksums](docs/core_bios.md).

The following non-commercial Sega Genesis ROM is included with Stable Retro for testing purposes:
- [Airstriker](https://pdroms.de/genesis/airstriker-v1-50-genesis-game) by Electrokinesis

 [List of other included ROMs](docs/included_roms.md).

## Contributing & Support

[See CONTRIBUTING.md](https://github.com/Farama-Foundation/stable-retro/blob/master/CONTRIBUTING.md)

There is an effort to get this project to the [Farama Foundation Project Standards](https://farama.org/project_standards). These development efforts are being coordinated in the `stable-retro` channel of the Farama Foundation's Discord. Click [here](https://discord.gg/aPjhD5cf) for the invite

Feel free to reach out to the above Discord for any issues/suggestions/discussions related to stable-retro

## Supported specs:

Platforms:
- Windows 10, 11 (via WSL2)
- macOS 10.13 (High Sierra), 10.14 (Mojave)
- Linux (manylinux1). Ubuntu 22.04 is recommended

CPU with `SSE3` or better

Supported Pythons: 3.7 to 3.12

## Citation

```
@misc{stable-retro,
  author = {Poliquin, Mathieu},
  title = {Stable Retro, a maintained fork of OpenAI's gym-retro},
  year = {2025},
  publisher = {GitHub},
  journal = {GitHub repository},
  howpublished = {\url{https://github.com/Farama-Foundation/stable-retro}},
}
```

## Papers

List of papers mentioning stable-retro. If you want your paper to be added here please open a github issue.

*	[Exploration-Driven Generative Interactive Environments](https://arxiv.org/pdf/2504.02515)
*	[Gymnasium: A Standardized Interface for Reinforcement Learning Environments](https://openreview.net/pdf?id=qPMLvJxtPK)
*	[IPR-1: Interactive Physical Reasoner](https://arxiv.org/html/2511.15407v1)
*	[SAFE-SMART: Safety Analysis and Formal Evaluation using STL Metrics for Autonomous RoboTs](https://arxiv.org/html/2511.17781v1)
