## What is MAME 2003-Plus?
[![pipeline status](https://git.libretro.com/libretro/mame2003-plus-libretro/badges/master/pipeline.svg)](https://git.libretro.com/libretro/mame2003-plus-libretro/-/commits/master)

MAME 2003-Plus (also referred to as MAME 2003+ and mame2003-plus) is a libretro arcade system emulator core with an emphasis on high performance and broad compatibility with mobile devices, single board computers, embedded systems, and similar platforms.

In order to take advantage of the performance and lower hardware requirements of an earlier MAME architecture, MAME 2003-Plus began with the MAME 2003 codebase which is itself derived from xmame 0.78. Upon that base, MAME 2003-Plus contributors have backported support for an additional 350 games, as well as other functionality not originally present in the underlying codebase.

## What games are supported?
View our [live compatibility table](https://buildbot.libretro.com/compatibility_lists/cores/mame2003-plus/mame2003-plus.html) built using datmagic, a tool created by the MAME 2003-Plus team. Datmagic auto-generates a compatibility table based on the latest XML DAT file. See an incorrect entry? Create a new issue to let us know! This allows us to keep the compatibility table as accurate as possible by fixing it at the source.

**Authors:** MAMEdev, MAME 2003-Plus team, et al (see [LICENSE.md](https://raw.githubusercontent.com/libretro/mame2003-plus-libretro/master/LICENSE.md) and [CHANGELOG.md](https://raw.githubusercontent.com/libretro/mame2003-plus-libretro/master/CHANGELOG.md))

# Documentation
User documentation for MAME 2003-Plus can be found in the **[libretro core documentation library](https://docs.libretro.com/)**.

Developer documentation can be found in **[the MAME 2003-Plus wiki](https://github.com/libretro/mame2003-plus-libretro/wiki)**.

## Development chat
#programming channel of the [libretro discord chat server](https://discordapp.com/invite/C4amCeV).

## Romsets and how to build them for this core

**mame2003-plus was originally built from the MAME 0.78 codebase, meaning that 95% or more of MAME 0.78 romsets will work as-is in mame2003-plus, where they immediately benefit from its bugfixes and other improvements.** In order to play the new games and games which received ROM updates in mame2003-plus, you will need to find or build the correct romsets.

**[Read more about rebuilding romsets in the libretro core documentation for mame2003-plus](https://docs.libretro.com/library/mame2003_plus/#Building-romsets-for-MAME-2003-Plus)**.
