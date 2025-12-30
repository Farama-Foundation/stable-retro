# Linux Installation

This guide covers installing Stable Retro on Linux distributions.

## Requirements

- Python 3.7 to 3.12
- Linux (manylinux1 compatible)
- CPU with SSE3 or better
- Ubuntu 22.04 is recommended

## Standard Installation

For most users, the simplest installation method is via pip:

```shell
pip3 install stable-retro
```

If the above doesn't work for your distribution:

```shell
pip3 install git+https://github.com/Farama-Foundation/stable-retro.git
```

## Development Installation

If you plan to integrate new ROMs, states or emulator cores, or plan to edit an existing environment:

```shell
git clone https://github.com/Farama-Foundation/stable-retro.git
cd stable-retro
pip3 install -e .
```

## Ubuntu/Debian Installation

### System Dependencies

Install the required system packages:

```shell
sudo apt-get update
sudo apt-get install python3 python3-pip git zlib1g-dev libopenmpi-dev ffmpeg
```

### With Stable Baselines 3

For reinforcement learning with Stable Baselines 3:

```shell
pip3 install git+https://github.com/Farama-Foundation/stable-retro.git
pip3 install stable_baselines3[extra]
```

## Building from Source

For development or to access the latest features:

### Install Build Dependencies

```shell
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config libzip-dev liblua5.3-dev \
    qtbase5-dev capnproto libcapnp-dev
```

### Nintendo 64 Core (Parallel N64)

The N64 core requires OpenGL headers to build. Install them with:

```shell
sudo apt-get install libgl1-mesa-dev
```

On Fedora/RHEL:
```shell
sudo dnf install mesa-libGL-devel
```

On Arch Linux:
```shell
sudo pacman -S mesa
```

If OpenGL headers are not found, the N64 core will be skipped during build. You can also explicitly disable it with `-DBUILD_N64=OFF`.

### Dreamcast Core (Flycast)

The Flycast core requires hardware rendering support. It uses EGL or GLX for headless OpenGL rendering.

**Additional dependencies:**

```shell
sudo apt-get install libegl1-mesa-dev libgles2-mesa-dev
```

**Required environment variable for headless rendering:**

When running on a headless server (no display), set:

```shell
export DISPLAY=:0
```

Or use a virtual framebuffer:

```shell
Xvfb :99 -screen 0 640x480x24 &
export DISPLAY=:99
```

Build with hardware rendering enabled:

```shell
pip install -e . -DENABLE_HW_RENDER=ON
```

### Clone and Build

```shell
git clone https://github.com/Farama-Foundation/stable-retro.git
cd stable-retro
pip3 install -e .
```

### Build Integration UI (Optional)

To build the game integration UI:

```shell
cmake . -DBUILD_UI=ON -UPYLIB_DIRECTORY
make -j$(nproc)
./gym-retro-integration
```

## Windows WSL2

Stable Retro works well on Windows 11 WSL2 with Ubuntu.

See the video tutorial: [Windows WSL2 + Ubuntu 22.04 setup guide](https://www.youtube.com/watch?v=vPnJiUR21Og)

## Verify Installation

After installation, verify it works by running:

```python
import stable_retro
env = stable_retro.make(game='Airstriker-Genesis-v0')
print("Installation successful!")
env.close()
```

## Troubleshooting

### Common Issues

- **Import errors**: Ensure you have the correct Python version (3.7-3.12)
- **Missing shared libraries**: Install the required system dependencies listed above
- **Permission errors**: You may need to use `pip3 install --user` or a virtual environment
- **32-bit systems**: Not supported due to compatibility issues with some cores

### Virtual Environment (Recommended)

Using a virtual environment helps avoid dependency conflicts:

```shell
python3 -m venv retro-env
source retro-env/bin/activate
pip install stable-retro
```
