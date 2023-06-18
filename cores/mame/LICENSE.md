
# MAME 2003-Plus License
#### MAME  -  Multiple Arcade Machine Emulator

Copyright (C) 1997-2003 by Nicola Salmoria and The MAME Team, Copyright (C)2003-2018 by the Libretro MAME 2003 Team and Copyright (C)2017-2018 by the Libretro MAME 2003-Plus Team. Backports from other versions of MAME are as noted in individual source files.

Many people have helped with this project--directly, or by releasing the source code for the drivers they have written. We are not trying to take credit that isn't ours. See the **Acknowledgments** section for a list of contributors. Please note, however, that the list is incomplete. Also see the comments in the source code to see the people who contributed to specific drivers. That list, too, may be incomplete. We apologize for any omission.

MAME 2003-Plus is licensed under the classic MAME Non-Commercial license and specifically the license of MAME 0.78 unless specifically stated otherwise by individual source files.

All trademarks cited in this document are property of their respective owners.

## MAME 0.78 Usage and Distribution License

### I. Purpose

MAME is strictly a non-profit project. Its main purpose is to be a reference to the inner workings of the emulated arcade machines. This is done for educational purposes and to prevent many historical games from sinking into oblivion once the hardware they run on stops working. Of course to preserve the games, you must also be able to actually play them; you can consider that a nice side effect.
   
It is not our intention to infringe on any copyrights or patents on the original games. All of MAME's source code is either our own or freely available. To operate, the emulator requires images of the original ROMs from the arcade machines, which must be provided by the user. No portions of the original ROM codes are included in the executable.

-------------------------

### II. Cost

MAME is free. Its source code is free. Selling either is not allowed.

-------------------------

### III. ROM Images

ROM images are copyrighted material. Most of them cannot be distributed freely. Distribution of MAME on the same physical medium as illegal copies of ROM images is strictly forbidden.
   
You are not allowed to distribute MAME in any form if you sell, advertise, or publicize CD-ROMs or other media containing illegal copies of ROM images. This restriction applies even if you don't make money, directly or indirectly, from those activities. You are allowed to make ROMs and MAME available for download on the same website, but only if you warn users about the ROMs's copyright status, and make it clear that users must not download ROMs unless they are legally entitled to do so.

-------------------------

### IV. Source Code Distribution

If you distribute the binary (compiled) version of MAME, you should also distribute the source code. If you can't do that, you must provide a link to a site where the source can be obtained.

-------------------------

### V. Distribution Integrity

This chapter applies to the official MAME distribution. See below for limitations on the distribution of derivative works. MAME must be distributed only in the original archives. You are not allowed to distribute a modified version, nor to remove and/or add files to the archive.

-------------------------

### VI. Reuse of Source Code

This chapter might not apply to specific portions of MAME (e.g. CPU emulators) which bear different copyright notices. The source code cannot be used in a commercial product without the written authorization of the authors. Use in non-commercial products is allowed, and indeed encouraged.  If you use portions of the MAME source code in your program, however, you must make the full source code freely available as well.

Usage of the _information_ contained in the source code is free for any use. However, given the amount of time and energy it took to collect this information, if you find new information we would appreciate if you made it freely available as well.

-------------------------

### VII. Derivative Works

Derivative works are allowed, provided their source code is freely available. However, these works are discouraged. MAME is a continuously-evolving project. It is in your best interests to submit your contributions to the MAME development team, so they may be integrated into the main distribution.

There are some specific modifications to the source code which go against the spirit of the project. They are NOT considered a derivative work, and distribution of executables containing them is strictly forbidden. Such modifications include, but are not limited to:
   - enabling games that are disabled
   - changing the ROM verification commands so that they report missing games
   - removing the startup information screens
   
If you make a derivative work, you are not allowed to call it MAME. You must use a different name to make clear that it is a MAME derivative, not an official distribution from the MAME team. Simply calling it MAME followed or preceded by a punctuation mark (e.g. MAME+) is not sufficient. The name must be clearly distinct (e.g. REMAME). The version number must also match the number of the official MAME version from which you derived your version.

-------------------------

## MAME 0.78 Acknowledgments
First of all, thanks to Allard van der Bas (avdbas@wi.leidenuniv.nl) for starting the Arcade Emulation Programming Repository at http://valhalla.ph.tn.tudelft.nl/emul8

Without the Repository, I would never have even tried to write an emulator. Unfortunately, the original Repository is now closed, but its spirit lives on in MAME.

* Z80 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
* M6502 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
* Hu6280 Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net
* I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
* M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
* M6808 based on L.C. Benschop's 6809 Simulator V09.
* M68000 emulator Copyright 1999 Karl Stenerud.  All rights reserved.
* 80x86 M68000 emulator Copyright 1998, Mike Coates, Darren Olafson.
* 8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.
* T-11 emulator Copyright (C) Aaron Giles 1998
* TMS34010 emulator by Alex Pasadyn and Zsolt Vasvari.
* TMS9900 emulator by Andy Jones, based on original code by Ton Brouwer.
* Cinematronics CPU emulator by Jeff Mitchell, Zonn Moore, Neil Bradley.
* Atari AVG/DVG emulation based on VECSIM by Hedley Rainnie, Eric Smith and Al Kossow.
* TMS5220 emulator by Frank Palazzolo.
* AY-3-8910 emulation based on various code snippets by Ville Hallik, Michael Cuddy, Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
* YM-2203, YM-2151, YM3812 emulation by Tatsuyuki Satoh.
* POKEY emulator by Ron Fries.
* Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge for information on the Pokey random number generator.
* NES sound hardware info by Matthew Conte.
* YM2610 emulation by Hiromitsu Shioya.
* Background art by Peter Hirschberg (PeterH@cronuscom.com).
* Allegro library by Shawn Hargreaves, 1994/97
* SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
* Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote TwkUser.c.
* "inflate" code for zip file support by Mark Adler.
* DOS executable compressed with UPX by Markus F.X.J. Oberhumer & Laszlo Molnar, http://upx.sourceforge.net/
* Big thanks to Gary Walton (garyw@excels-w.demon.co.uk) for too many things to mention.
* Thanks to Brian Deuel, Neil Bradley, and the Retrocade dev team for allowing us to use Retrocade's game history database.
* Thanks to Richard Bush for info on several games.
* Thanks to Dave (www.finalburn.com) for info on After Burner.

and thanks to everyone else I forgot.
