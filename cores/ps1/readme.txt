
PCSX-ReARMed - yet another PCSX fork, ARM special

http://notaz.gp2x.de/pcsx_rearmed.php


About
-----

PCSX ReARMed is yet another PCSX fork based on the PCSX-Reloaded project,
which itself contains code from PCSX, PCSX-df and PCSX-Revolution. This
version is ARM architecture oriented and features MIPS->ARM recompiler by
Ari64, NEON GTE code and more performance improvements. It was created for
Pandora handheld, but should be usable on other devices after some code
adjustments (N900, GPH Wiz/Caanoo, PlayBook versions are also available).

PCSX ReARMed features ARM NEON GPU by Exophase, that in many cases produces
pixel perfect graphics at very high performance. There is also Una-i's GPU
plugin from PCSX4ALL project, and traditional P.E.Op.S. one.


Compiling
---------

For libretro build, just doing "make -f Makefile.libretro" is recommended as
it's the way libretro team is building the core and only Makefile.libretro is
maintained by them.

For standalone build, './configure && make' should work for the most part.

When compiling for ARM, it's advisable to tell configure script the CPU, FPU
and ABI that matches your target system to get best performance, like this:

CFLAGS='-mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp' ./configure

Cross compilation should also work if kernel-style CROSS_COMPILE variable
is set:
CROSS_COMPILE='arm-none-linux-gnueabi-' ./configure


Usage
-----

There are several different frontends that can be built from source (one
generic and several platform specific), so usage slightly differs depending
on that. Most of them have a menu that can be used to run games and configure
the emulator.

Supported CD image formats:
- .bin/.cue
- .bin/.toc
- .chd
- .img/.ccd/.sub
- .mdf/.mds
- EBOOT.PBP (PSP, partial)
- .cbn

CDDA (CD audio) only supported when .cue/.toc/.ccd/.mds files are present.
There is support for redump.org .sbi files, which can be used instead of
.sub files to save space (name it the same as .cue/.bin, just use .sbi
extension). This is required for Libcrypt copy protected game support.

The emulator can simulate BIOS, which means BIOS files are not required,
however implementation is not complete and some games still need real BIOS
to work. To use real BIOS, copy uncompressed BIOS files to bios/ directory
which itself should be in main emulator directory.

For pandora, it should be:
[sd card]/pandora/appdata/pcsx_rearmed/bios/

When the file is copied, BIOS should be selected in Options->BIOS/Plugins menu.

On pandora, analog controllers are supported using nubs, but this is
disabled by default and needs to be enabled in 'Controls' menu.
There is also touchscreen based GunCon support, which also requires
appropriate controller selected in controls configuration.


Plugins
-------

GPU (graphics) and SPU (sound) plugins can be selected in
[BIOS/Plugins] menu:

builtin_gpu    - this is either Exophase's ARM NEON GPU (accurate and fast,
                 available if platform supports NEON, like on pandora),
                 gpu_peops or gpu_unai (depends on compile options).
gpu_neon.so    - Exophase's ARM NEON GPU with ARMv7 assembly optimizations.
gpu_peops.so   - P.E.Op.S. soft GPU, reasonably accurate but slow
                 (also found with older emulators on PC)
gpu_gles.so    - experimental port of P.E.Op.S. MesaGL plugin to OpenGL ES.
                 Occasionally faster but has lots of glitches and seems to
                 be rather unstable (may crash the driver/system).
builtin_spu    - P.E.Op.S. SPU plugin, optimized for ARM.
spunull.so     - NULL plugin, i.e. no sound emulation.
                 May cause compatibility problems.


Cheats
------

PCSX and cwcheat cheat formats are supported. PCSX .cht file can be loaded from
"extra stuff" menu after game is loaded, while cwcheat cheats are automatically
loaded from cheatpops.db file, if one is present in emulator's directory and
has any cheats for the loaded game in it.
If any of those files are successfully loaded, 'cheats' option will appear in
the main menu where it is possible to enable/disable individual cheats.


Changelog
---------

r25 (2025-02-26)
* bug fixes and compatibility improvements

r24 (2024-01-22)
* HLE compatibility has been greatly improved
* various compatibility and accuracy improvements
+ support for more lightgun types
- savestates from older versions might not work, especially if HLE was used.
  Memory card files remain fully compatible.
* many small changes I forgot about

r23 (2022-03-14)
* many fixes from various contributors on github and from the libretro fork
* dynarec related slowdowns have been greatly reduced
* many dynarec bug fixes
+ added chd disk image support

r22 (2015-02-05)
* general: fixed a race condition/crash in threaded SPU mode
* pandora: C64x: fixed compatibility with newer c64_tools, enabled L2 cache
* frontend: fixed control config corruption on load for devices that are
  disconnected on startup
* some dma accuracy improvements, might fix occasional glitches in ff7
* ARMv6 build and the dynarec now make use of available instructions (gizmo98)

r21 (2015-01-12)
+ general: added ability to run SPU emulation on a separate thread, enabled it
  by default when multicore CPU is detected. Significant effort was made to
  avoid any compatibility problems which the old P.E.Op.S. implementation had.
+ pandora: added ability to run SPU emulation on TI C64x DSP by using bsp's
  c64_tools.
* libretro: fixed win32 build (mingw only)
* some tweaks for the scanline effect and other things

r20 (2014-12-25)
* fixed various sound accuracy issues, like effects in ff7-ff9
  for standalone build, audio will no longer slow down when emu is not fast
  enough and stutter instead, as the former behavior causes accuracy issues.
  Old mode can be restored in SPU plugin config options, but is not recommended.
* savestates now save small parts of dynarec state to reduce dynarec related
  slowdowns after savestate load
* menu: fixed file browser issues with filesystems like exfat-fuse
* menu: memcard manager: selected card is saved in config now
* standalone: added some basic scanline efect
* some CD image loading fixes
* converted asm code to be compatible with more assemblers, like Apple's gas
+ libretro: added Makefile.libretro and support for various platforms like
  iOS and QNX. Makefile.libretro is recommended way to do libretro builds
  (patches from CatalystG, squarepusher, notaz and others, see git).
* some other minor fixes

r19 (2013-03-17)
+ libretro: added region, multidisk support
* more work on cdrom code
* changed sound sync code
* fixed masking bugs in NEON GPU (in collaboration with Exophase)
* fixed some compatibility issues
* various other tweaks and fixes

r18 (2013-01-06)
* cdrom code greatly cleaned up
+ new GLES output mode for ARM Linux/R-Pi
* various libretro improvements
* fixed several compatibility regressions
* various other tweaks and fixes

r17 (2012-11-24)
+ added overlay support for generic Linux build
* attempted to fix sound breakage with PulseAudio
* fixed some regressions caused by hires mode code
* fixed some sound issues introduced in r9
* various other tweaks

r16 (2012-11-10)
+ gpu_neon now has new hires rendering mode
  (sometimes slow and occasionally glitchy)
+ integrated M-HT's scale2x and eagle2x filters
  (works for 512x256 or lower resolution games)
+ pandora: added gamma/brightness control (requires SZ 1.52)
* pandora: some improvements for nub support
+ added fast forward support
* fixed some glitches after loading savestates from r14 or earlier

r15 (2012-08-02)
* various compatibility fixes
* attempts to fix various SPU issues
* Exophase fixed blending issue in his NEON GPU
* fixed some potential crashes
* gpu_unai: merged range fix from Franxis
+ added cheat support
+ menu: pressing a key in file list now seeks to a file
+ new code, fixes and refactoring to improve portability:
  support RAM offset, translation cache in data segment,
  SDL support, multiple sound output methods, configure script
* unified plugin names for all ports
+ initial libretro support

r14 (2012-03-04)
* GLES GPU: implemented frameskip
* GLES GPU: merged some changes from schtruck/FPSE
* Caanoo: potential workaround for save corruption
  (always exit emulator cleanly before turning off the console
   to reduce chance of corruption)
* Caanoo: fixed a bug in GTE code (graphic glitches in some games)
* Caanoo: reworked vibration support, should support more games
* various refactoring/minor tweaks

r13 (2012-01-09)
* yet more fixes for regressions from earlier versions
* various fixes for NEON GPU (in collaboration with Exophase)
+ NEON GPU supports interlace mode now, but it's not always
  enabled due to frameskip glithes (can be changed in the menu)
* cdda should resume on savestate load now
* fixed date display in menus to honour locale
+ pandora: added support for minimizing the emulator (while ingame only)

r12 (2011-12-24)
+ new ARM NEON GPU rasterizer from Exophase (NEON hardware required)
+ new GPU emulation code
+ new analog controller configurator
* changed frameskip handling (again..), higher values supported
* fixed several more regressions from earlier versions
* changed cdrom code with hope for better compatibility
* sprite optimization for PCSX4ALL plugin
+ Caanoo: added vibration support

r11 (2011-10-31)
+ added Wiz support
* Caanoo: fixed tv-out
+ Caanoo/Wiz: added scaling (16bpp only)
+ Caanoo/Wiz: added touchscreen-as-buttons input (4 sections)
+ added .cbin support
+ added multidisk eboot support (use "next multidisk CD" in exras menu)
* some GTE related optimizations
* various other optimizations
+ added some speed hack options for slower devices
  (get more speed at stability and correctness loss)
* fixed several compatibility issues
* fixed a few crash situations
* various minor adjustments
* maemo: merged some code from Bonapart
* maemo: fixed BIOS issue (hopefully)

r10 (2011-10-10)
+ added Caanoo port
+ completely rewrote memory handlers
+ added fixed frameskip option
+ added ability to change PSX clock
+ implemented GTE dead flag detection
* switched to larger timeslices for better performance
* fixed some cases of flickering
* fixed a crash in PCSX4ALL GPU plugin
* fixed several dynarec compatibility related issues (hopefully)
* fixed multiple SPU regressions from r9 and earlier
* fixed frame limiter issue that sometimes caused stuttering
* fixed some minor GUI issues

r9 (2011-08-13)
* fixed various dynarec integration issues that were causing instability
* merged latest Ari64 dynarec code for some performance improvement
* changed frameskip handling in builtin and PCSX4ALL plugins,
  fixes some cases where it would not work
* merged PCSX4ALL 2.2 GPU code to it's plugin
* fixed PCSX4ALL GPU inline asm, was miscompiling for ARMv7.
+ added CDDA handling for eboot format
* improved CDDA handling for all image formats that support it
* various compatibility/accuracy improvements
* optimized PEOPS SPU core
* various menu adjustments
* changed scaling options a bit, there are now two 4:3 options:
  integer and fractional
+ added some basic memory card manager, which allows to change
  or remove cards (remove needed for Tenka)
+ added GunCon support
+ added gpuPEOPS2 plugin (peops rendering + new emulation code)

r8 (2011-03-22)
* improved recompiler performance for some games
* fixed a few recompiler related compatibility issues
  (also fixes broken memcard support in some games)
* fixed some graphics problems caused by frameskip.
  Note that not all problems were fixed, so if you see graphics
  glitches try turning off frameskip or using different GPU plugin.
+ added screenshot function
+ added some code to attempt to sync with pandora's LCD better
* merged a few compatibility fixes from PCSX-Reloaded
* fixed and issue with external controllers
* added experimental ability to use nubs as buttons

r7 (2011-03-02)
+ implemented most used GTE operations in NEON
* merged latest Ari64's recompiler patches
* removed some code from the recompiler that is unneeded for R3k
* added some special handlers for constant reads
* some moderate builtin GPU and SPU optimizations
+ added redump.org SBI support
* tuned frameskip code again
* fixed one 'analog controller not working' issue
* fixed a crash in builtin gpu code
* fixed cdrom slowdown issue
* fixed my stupid bug in the recompiler that slowed down
  recompilation a lot
* some other refactoring

r6 (2011-02-10)
+ added analog controller support using nubs (disabled by default)
+ added control config saving
+ added support for ingame actions (eg. savestate load)
+ added 'auto' region option and made it default
+ added cd swap functionality
+ added maemo frontend from Bonapart
  (with some tuning, source code only)
* reworked key configuration to be less confusing
* fixed 'SPU IRQ wait' option sometimes causing noise
  and turned it on by default
* fixed mono xa masking (was causing noise)
* fixed word access macros in dfxvideo (darkness problem)
* changed GPU DMA timing back to 1.92 levels
* backported more fixes from PCSX-Reloaded project
  (mostly shalma's work, see GIT)
* fixed a few more recompiler issues
+ fixed frameskip in builtin plugin

r5 (2011-01-31)
+ added support for .bz format, also partial support for
  .znx and eboot.pbp formats
+ merged latest cdrom code from PCSX-Reloaded project
* fixed remaining savestate incompatibilities between PCSX4ALL
  and P.E.Op.S. GPU plugins
* fixed channel disable preventing irqs in P.E.Op.S. SPU plugin
* fixed some alignment issues
+ added handling for branches in delay slots
+ fixed some unexpected drops to menu
* fixed lots of recompiler related issues (see GIT)
+ added watchdog thread to detect emulator lockups
* minor frontend adjustments

r4 (2011-01-15)
+ added real BIOS support (and various things for it to work)
* fixed various recompiler issues
+ added interpreter option (useful to overcome dynarec bugs)
* fixed some memory card related issues with HLE bios
* rewrote frame limiter (old was sometimes sleeping needlessly)

r3 (2011-01-05):
+ added Pickle's port of gpu-gles from psx4m project
+ added PCSX4ALL gpu as a plugin
* improved gpu plugin support
+ added savestate preview
* various frontend fixes

r2 (2010-12-29):
* fixed memcard paths
* fixed a keybind copy-paste bug
* properly implemented pad handling
  (inputs no longer control both emulated pads at once)
* fixed a crash caused by framebuffer out of range access
* fixed SWL/SWR handling (usually resulted in graphic glitches)
* fixed BxxZAL (Medal of Honor)
* fixed alignment crash in color space conversion code (Lunar)
* fixed SWC2 occasional use of wrong address register (Parasite Eve)
* fixed firstfile() handling in HLE BIOS (broken memory cards in some games)
+ added per-game configs (controls still not saved though)
+ added simple plugin select interface to the menu

r1 (2010-12-25):
* initial release


Credits / License
-----------------

Emulator core:

(C) 1999-2003 PCSX Team
	(c) 1998 Vision Thing
	Linuzappz     <linuzappz@pcsx.net>
	Shadow        <shadow@pcsx.net>
	Pete Bernett  <psswitch@online.de>
	NoComp        <NoComp@mailcity.com>
	Nik3d
	Akumax        <akumax@pcsx.net>

(C) 2005-2009 PCSX-df Team
	(c) Ryan Schultz <schultz.ryan@gmail.com>
	(c) Andrew Burton <adb@iinet.net.au>
	(c) Stephen Chao <schao@myrealbox.com>
	(c) Marcus Comstedt <marcus@mc.pp.se>
	Stefan Sikora <hoshy@schrauberstube.de>

(C) 2009-2011 PCSX-Reloaded Team
	edgbla (Root counters, various core/plugin fixes)
	shalma (GTE Divider, many core improvements, sound plugin fixes)
	Firnis (GTE code from PCSX-Revolution Project)
	Gabriele Gorla (MDEC decoder)
	Peter Collingbourne (Various core/psxbios fixes)
	Dario, NeToU, siveritas (Various bugfixes)
	Wei Mingzhi (Maintainer, input plugin, iso/cheat support, misc stuff)

NEON GPU plugin:
	(C) 2011-2012 Exophase
	(C) 2011-2012 notaz

PCSX4ALL GPU plugin:
	(C) 2010 PCSX4ALL Team
	(C) 2010 Unai
	Franxis <franxism@gmail.com>
	Chui <sdl_gp32@yahoo.es>

GLES plugin (psx4m project):
	(C) 1999-2009 by Pete Bernert
	EQ
	Olli Hinkka
	Proger
	Pickle

P.E.Op.S. GPU plugin:
	(C) Pete Bernert and the P.E.Op.S. team

P.E.Op.S. SPU plugin:
	(C) Pete Bernert and the P.E.Op.S. team
	(C) SPU2-X, gigaherz, Pcsx2 Development Team
	shalma
	notaz

MIPS->ARM recompiler:
	(C) 2009-2011 Ari64

integration, optimization and frontend:
	(C) 2010-2012 notaz

Special thanks to Mednafen author, shalma/gretar and Rokas for
various help while developing this emulator.

Some implementation ideas (and maybe code?) likely originated from
MAME/smf/pSXauthor and were integrated by various people to PCSX.

Source code is released under GNU GPL license, version 2 or later.
See COPYING included in the archive (pandora version's .pnd can be
extracted using unsquashfs).

The source code is available in a GIT repository at:

git://notaz.gp2x.de/~notaz/pcsx_rearmed.git

