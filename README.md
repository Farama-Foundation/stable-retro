[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://pre-commit.com/) [![Code style: black](https://img.shields.io/badge/code%20style-black-000000.svg)](https://github.com/psf/black)

# Stable-Retro

A fork of [gym-retro](https://github.com/openai/retro) ('lets you turn classic video games into Gymnasium environments for reinforcement learning') with additional games, emulators and supported plateforms. Since gym-retro is in maintenance now and doesn't accept new games, plateforms or bug fixes, you can instead submit PRs with new games or features here in stable-retro.

Currently added games on top of gym-retro:
*	Super Mario Bros 2 Japan (Lost Levels) - NES
*	Hang On - SMS
*	Punch Out - NES
*	WWF Wrestlemania the Arcade Game - Genesis
*	NHL 94 - Genesis
*	NHL 94 (1 on 1 rom hack) - Genesis
*	Super Hang On - Genesis
*	Tetris - GameBoy
*	Virtua Fighter 2 - Genesis
*	Virtua Fighter - 32x

PvP games that support two models fighting each other:
*	Samurai Showdown - Genesis
*	WWF Wrestlemania the Arcade Game - Genesis
*	Mortal Kombat II - Genesis
*	NHL 94 - Genesis

As well as additional states on already integrated games.

## Emulated Systems

- Atari
	- Atari2600 (via Stella)
- NEC
	- TurboGrafx-16/PC Engine (via Mednafen/Beetle PCE Fast)
- Nintendo
	- Game Boy/Game Boy Color (via gambatte)
	- Game Boy Advance (via mGBA)
	- Nintendo Entertainment System (via FCEUmm)
	- Super Nintendo Entertainment System (via Snes9x)
- Sega
	- GameGear (via Genesis Plus GX)
	- Genesis/Mega Drive (via Genesis Plus GX)
	- Master System (via Genesis Plus GX)
        - 32x (via Picodrive)

## Bug Fixes
*	Fixed UI flickering issue in OpenAI integration tool
*	fix compile with c++ >=17

## Installation

```
pip3 install git+https://github.com/Farama-Foundation/stable-retro.git
```
#### Apple Silicon Installation (Tested on python3.10)
- NOTE: The Game Boy (gambatte) emulator is not supported on Apple Silicon

**Build from source**
1. `pip install cmake wheel`
2. `brew install pkg-config lua@5.1 libzip qt5 capnp`
3. `echo 'export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"' >> ~/.zshrc`
4. `export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)`
5. `pip install -e .`

**Build Integration UI**
1. build package from source
2. `cmake . -DCMAKE_PREFIX_PATH=/usr/local/opt/qt -DBUILD_UI=ON -UPYLIB_DIRECTORY`
3. `make -j$(sysctl hw.ncpu | cut -d: -f2)`
4. `open "Gym Retro Integration.app"`


Video on how to setup on Ubuntu and Windows:
https://youtu.be/LRgGSQGNZeE

Docker image for M1 Macs:
https://github.com/arvganesh/stable-retro-docker

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
python3 ppo.py --game='Airstriker-Genesis'
```

## Citation

```
@misc{stable-retro,
  author = {Mathieu and Poliquin},
  title = {Stable Retro, a fork of OpenAI's gym-retro},
  year = {2021},
  publisher = {GitHub},
  journal = {GitHub repository},
  howpublished = {\url{https://github.com/Farama-Foundation/stable-retro}},
}
```

## Tutorials

Game Integration tool:
https://www.youtube.com/playlist?list=PLmwlWbdWpZVvWqzOxu0jVBy-CaRpYha0t

## Discord channel

Join here:
https://discord.gg/dXuBSg3B4D

## Contributing

[See CONTRIBUTING.md](https://github.com/Farama-Foundation/stable-retro/blob/master/CONTRIBUTING.md)

There is an effort to get this project to the [Farama Foundation Project Standards](https://farama.org/project_standards). These development efforts are being coordinated in the `stable-retro` channel of the Farama Foundation's Discord. Click [here](https://discord.gg/aPjhD5cf) for the invite

## Supported specs:

Plateforms:
- Windows 10, 11 (via WSL2)
- macOS 10.13 (High Sierra), 10.14 (Mojave)
- Linux (manylinux1)

CPU with `SSSE3` or better

Supported Pythons:

- 3.7
- 3.8
- 3.9
- 3.10

## Documentation

Documentation is available at [https://stable-retro.farama.org/](https://stable-retro.farama.org/) (work in progress)

See [LICENSES.md](https://github.com/Farama-Foundation/stable-retro/blob/master/LICENSES.md) for information on the licenses of the individual cores.

## ROMs

Each game integration has files listing memory locations for in-game variables, reward functions based on those variables, episode end conditions, savestates at the beginning of levels and a file containing hashes of ROMs that work with these files.

Please note that ROMs are not included and you must obtain them yourself.  Most ROM hashes are sourced from their respective No-Intro SHA-1 sums.

The following non-commercial ROMs are included with Stable Retro for testing purposes:

- [the 128 sine-dot](http://www.pouet.net/prod.php?which=2762) by Anthrox
- [Sega Tween](https://pdroms.de/files/gamegear/sega-tween) by Ben Ryves
- [Happy 10!](http://www.pouet.net/prod.php?which=52716) by Blind IO
- [512-Colour Test Demo](https://pdroms.de/files/pcengine/512-colour-test-demo) by Chris Covell
- [Dekadrive](http://www.pouet.net/prod.php?which=67142) by Dekadence
- [Automaton](https://pdroms.de/files/atari2600/automaton-minigame-compo-2003) by Derek Ledbetter
- [Fire](http://privat.bahnhof.se/wb800787/gb/demo/64/) by dox
- [FamiCON intro](http://www.pouet.net/prod.php?which=53497) by dr88
- [Airstriker](https://pdroms.de/genesis/airstriker-v1-50-genesis-game) by Electrokinesis
- [Lost Marbles](https://pdroms.de/files/gameboyadvance/lost-marbles) by Vantage
