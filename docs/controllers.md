# Controllers

Stable Retro supports one to four player input channels. The default is one
player, and omitting `controller_devices` preserves the emulator core's default
controller setup. Increasing `players` changes the action space; it does not
automatically enable a console's multitap or other adapter.

## Selecting controller devices

Use `controller_devices` to map zero-based physical connector numbers to
core-advertised device IDs:

```python
import stable_retro as retro

env = retro.make(
    "NHL94-Genesis-v0",
    players=4,
    controller_devices={0: 1025},
)
try:
    print(env.em.controller_devices)
    observation, info = env.reset()
    observation, reward, terminated, truncated, info = env.step(
        env.action_space.sample(),
    )
finally:
    env.close()
```

For the Genesis core, device `1025` is the three-button EA 4-Way Play adapter.
The core advertises two physical connectors; selecting this adapter on connector
0 enables four input channels across both connectors. The game and selected
savestate must also support the adapter and the desired multiplayer mode.
Device IDs and available adapters are core-specific.

`env.em.controller_devices` returns a list of dictionaries, one per advertised
connector, mapping device IDs to descriptions. The low-level method
`env.em.set_controller_port_device(port, device)` rejects invalid connectors and
devices that the core does not advertise. For a core without controller
metadata, only the standard joypad (`1`) and disconnected (`0`) devices can be
selected.

The `controller_devices` constructor argument configures devices after loading
the ROM and before restoring the starting state. Changing a device resets its
protocol state, so do not repeat device setup after restoring a gameplay
snapshot. The Genesis core preserves its chosen adapter across reset and state
restore, while restoring the saved console I/O state separately. A savestate
made without an adapter may require game-specific initialization of its
controller-polling registers; enabling the adapter alone cannot repair it.

## Player inputs and rewards

`players` must be an integer from `1` through `RetroEmulator.max_players`
(currently `4`). Player input channels are zero-based and distinct from physical
connectors.

The default `MultiBinary` action space has `env.num_buttons * players` entries,
grouped by player: all player 1 buttons, then all player 2 buttons, and so on.
`Actions.DISCRETE` and `Actions.MULTI_DISCRETE` also scale with the player count.
The low-level `env.em.set_button_mask(mask, player)` sets one input channel,
not a physical connector. Unused channels should remain neutral.

The Gymnasium reset and step return formats are unchanged. Rewards remain
scalar by default, including in multiplayer environments. Setting
`env.multi_rewards = True` returns a reward per player, using the integration's
configured rewards.

Native input callbacks support individual joypad buttons and packed joypad
bitmasks. Unsupported device types, indices, channels, and buttons return
neutral input. Python button masks must be contiguous one-dimensional vectors
with at most 16 buttons; omitted buttons are released.

## Movies and replay

BK2 movies support one to four players. Movie reads and writes reject player
indices outside the movie's player count and button indices outside `0` through
`15`.

BK2 files do not encode controller-device configuration. Record that
configuration separately and pass the same `controller_devices` mapping when
creating the replay environment, before restoring the movie state. Match
`players` to `movie.players`; specifying four players alone does not select an
adapter. See {doc}`python` for recording and playback examples.

## Regression tests

Controller tests use the bundled Genesis homebrew ROM; no commercial ROM is
required:

```shell
python -m pytest tests/test_python/test_controllers.py
```

They cover device metadata and validation, input bounds, four-player movies,
and environment contracts. Native callback coverage is included in
`EmulatorTest.ControllerInputBoundsAndBitmasks`.
