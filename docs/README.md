<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [Stable-retro docs](#stable-retro-docs)
  - [Instructions for modifying environment pages](#instructions-for-modifying-environment-pages)
  - [Build the Documentation](#build-the-documentation)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Stable-retro docs

This folder contains the documentation for [StableRetro](https://github.com/Farama-Foundation/stableretro).

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
