PCSX-ReARMed - yet another PCSX fork
====================================

![CI (Linux)](https://github.com/notaz/pcsx_rearmed/workflows/CI%20(Linux)/badge.svg)

*see [readme.txt](readme.txt) for more complete documentation*

PCSX ReARMed is yet another PCSX fork based on the PCSX-Reloaded project,
which itself contains code from PCSX, PCSX-df and PCSX-Revolution. This
version is ARM architecture oriented and features MIPS->ARM recompiler by
Ari64, NEON GTE code and more performance improvements. It was created for
Pandora handheld, but should be usable on other devices after some code
adjustments (N900, GPH Wiz/Caanoo, PlayBook versions are also available).

PCSX ReARMed features ARM NEON GPU by Exophase, that in many cases produces
pixel perfect graphics at very high performance. There is also Una-i's GPU
plugin from PCSX4ALL project, and traditional P.E.Op.S. one.


PCSX-Reloaded
=============

PCSX-Reloaded is a forked version of the dead PCSX emulator, with a nicer
interface and several improvements to stability and functionality.

PCSX-Reloaded uses the PSEMU plugin interface to provide most functionality;
without them, you will not be able to use it to play games. PCSX-Reloaded
provides a number of plugins to provide basic functionality out of the box.

PCSX-Reloaded has a very capable Internal HLE BIOS that can run many games
without problems. It is recommended that you use it. However, if you own a
real PlayStation, you may be able to use your own BIOS image. PCSX-Reloaded
will find it in ~/.pcsx/bios/ or /usr/share/psemu/bios/ if you place it there.
This can improve compatibility, especially with certain games and with the
use of memory cards.

See the doc/ folder in the source, or /usr/share/doc/pcsx/ on Debian systems,
for more detailed information on PCSX-Reloaded. A UNIX manpage is also
available.
