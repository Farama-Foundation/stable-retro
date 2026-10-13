# Stable Retro TyrQuake Notes

This source snapshot comes from <https://github.com/libretro/tyrquake> commit `e57bb11597e8a00380f30f2627d219da960cf69a`.

Stable Retro carries two local changes:

- The embedded version is pinned to the upstream snapshot instead of querying the enclosing Stable Retro Git checkout.
- `RETRO_MEMORY_SYSTEM_RAM` exposes the 32-entry `cl.stats` array, and a libretro memory map appends `cl.intermission` and `cl.completed_time`. Integrations can therefore read compact gameplay and level-completion statistics without scanning TyrQuake's engine heap.

Libretro save states remain unsupported upstream. No Quake game data is included.