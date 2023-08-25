# Stable-retro docs

This folder contains the documentation for [StableRetro](https://github.com/Farama-Foundation/stable-retro).

If you are modifying an atari environment page, please follow the instructions below. For more information about how to contribute to the documentation go to our [CONTRIBUTING.md](https://github.com/Farama-Foundation/Celshast/blob/main/CONTRIBUTING.md)

## Instructions for modifying environment pages

TODO - Add details

## Build the Documentation

Install the required packages and StableRetro (or your fork):

```
pip install stableretro
pip install -r docs/requirements.txt
```

To build the documentation once:

```
cd docs
make dirhtml
```

To rebuild the documentation automatically every time a change is made:

```
cd docs
sphinx-autobuild -b dirhtml . _build
```
