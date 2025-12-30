# macOS Installation

This guide covers installing Stable Retro on macOS, including both Intel and Apple Silicon Macs.

## Requirements

- Python 3.7 to 3.12
- macOS 10.13 (High Sierra) or later

## Standard Installation

For most users, the simplest installation method is via pip:

```shell
pip3 install stable-retro
```

If the above doesn't work for your platform:

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

## Apple Silicon Installation

Tested on Python 3.10.

```{note}
The Game Boy (gambatte) emulator is not supported on Apple Silicon.
```

### Build from Source

1. Install build dependencies:
   ```shell
   pip install cmake wheel
   ```

2. Install system dependencies via Homebrew:
   ```shell
   brew install pkg-config lua@5.3 libzip qt@5 capnp
   ```

3. Add Qt to your PATH:
   ```shell
   echo 'export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"' >> ~/.zshrc
   ```

4. Set the SDK root:
   ```shell
   export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
   ```

5. Install the package:
   ```shell
   pip install -e .
   ```

### Build Integration UI

After building from source, you can optionally build the Integration UI:

1. Configure CMake with UI support:
   ```shell
   cmake . -DCMAKE_PREFIX_PATH=/usr/local/opt/qt -DBUILD_UI=ON -UPYLIB_DIRECTORY
   ```

2. Build the application:
   ```shell
   make -j$(sysctl hw.ncpu | cut -d: -f2)
   ```

3. Launch the Integration UI:
   ```shell
   open "Gym Retro Integration.app"
   ```

## Docker for Apple Silicon

An alternative option for M1/M2/M3 Macs is to use Docker:

[https://github.com/arvganesh/stable-retro-docker](https://github.com/arvganesh/stable-retro-docker)

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
- **Build failures on Apple Silicon**: Make sure you've set `SDKROOT` correctly
- **Qt not found**: Verify Qt is in your PATH after running the export command
- **Game Boy games not working on Apple Silicon**: This is expected; the gambatte core is not supported on arm64
