# macOS Installation

Stable Retro supports native Intel and Apple Silicon builds with Python 3.11 through 3.14. The default source-build deployment target is macOS 10.13 on Intel and macOS 11.0 on Apple Silicon. Your Python distribution and optional dependencies may require a newer macOS version.

## Standard Installation

Use a virtual environment:

```shell
python3 -m venv .venv
source .venv/bin/activate
python -m pip install stable-retro
```

If a wheel is unavailable for your Python and architecture, pip builds from source. See the requirements below.

## Build from Source

Install Apple's command line tools if needed:

```shell
xcode-select --install
```

Use a native Python interpreter. On Apple Silicon, this should print `arm64`:

```shell
python -c 'import platform; print(platform.machine())'
```

Then build in your virtual environment:

```shell
git clone https://github.com/Farama-Foundation/stable-retro.git
cd stable-retro
python -m pip install -e .
```

Pip installs the Python build tools, including CMake. The default build bundles LuaJIT and falls back to vendored libzip when a system libzip is unavailable. Homebrew Lua, Qt, and Cap'n Proto are not required for headless RL training. Gambatte (Game Boy and Game Boy Color) is built on both Intel and Apple Silicon.

CMake normally selects the macOS SDK automatically. If SDK discovery fails, check `xcode-select -p` and, if necessary, set:

```shell
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
```

A custom deployment target can be passed through to CMake, for example:

```shell
CMAKE_ARGS="-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0" python -m pip install -e .
```

Build for the native architecture of your Python interpreter. Reusing a checkout previously built under Rosetta can leave incompatible CMake caches, core objects, or libraries; use a fresh checkout when changing architectures.

## Verify Headless Training

The bundled Airstriker ROM allows testing without importing a commercial game. Select `rgb_array` explicitly: the default `human` mode opens a pyglet window on reset and step.

```python
import stable_retro

with stable_retro.make(game="Airstriker-Genesis-v0", render_mode="rgb_array") as env:
    observation, info = env.reset(seed=0)
    for _ in range(120):
        observation, reward, terminated, truncated, info = env.step(
            env.action_space.sample()
        )
        if terminated or truncated:
            observation, info = env.reset()
    print("Headless emulation successful:", observation.shape)
```

Use this mode in RL workers as well. It returns CPU image observations without creating an OpenGL window; it does not require a display server or Xvfb on macOS.

## Build the Integration UI

The optional integration application still uses Qt 5. Install it separately, if available for your Homebrew installation:

```shell
brew install qt@5
cmake -S . -B build-ui -DBUILD_UI=ON \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)"
cmake --build build-ui --parallel 2
open "build-ui/Gym Retro Integration.app"
```

`brew --prefix` works with both Apple Silicon and Intel Homebrew paths. Qt 5 is deprecated upstream; availability is separate from the default Python package build.

## Troubleshooting and Limitations

- **Wrong architecture / incompatible Mach-O**: Check that Python, Homebrew dependencies, and compiled cores all use the same architecture. Avoid mixing `/usr/local` Intel libraries with `/opt/homebrew` ARM libraries.
- **A blank, flickering, or crashing human-rendering window**: The viewer and interactive example require pyglet 2.1 and OpenGL 3.3 or later. Upgrade the package dependencies in your virtual environment, and run the headless check above to isolate display failures from emulation failures. Run windows on the main thread in a macOS desktop session; keep RL workers in `rgb_array` mode.
- **Qt not found**: Qt is needed only for the integration UI. Pass its installation prefix as shown above.
- **Unavailable cores**: N64 and FBNeo are currently excluded from macOS builds. Hardware rendering and Dreamcast support currently require Linux.

## Test Window Rendering

The human viewer and interactive example share a shader-based image renderer. It uses nearest-neighbor scaling and supports rotated frames, window resizing, and displays where framebuffer pixels differ from window coordinates.

Run the OpenGL regression tests from a desktop session:

```shell
python -m pip install -e '.[dev]'
STABLE_RETRO_TEST_RENDERING=1 python -m pytest tests/test_python/test_rendering.py
```

These tests create temporary windows and check the rendered pixels, keyboard state, and window cleanup. They are skipped by default when the environment variable is absent. On Linux they can run under Xvfb; macOS requires a WindowServer session.

If you have imported a Tetris ROM, run its viewer check with:

```shell
STABLE_RETRO_TEST_RENDERING=1 python -m pytest tests/test_python/test_rendering.py -k Tetris
python -m stable_retro.examples.interactive --game Tetris-GameBoy-v0
```

The interactive controls include arrow keys for movement, Z/X for the game buttons, Enter for Start, and Escape to close the viewer.
