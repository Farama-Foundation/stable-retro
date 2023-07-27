# Beetle Saturn libretro

This is fork of Mednafen Saturn. It has been ported to the libretro API.
It currently runs on Linux, OSX and possibly Windows.

## Running

To run this core, the "system directory" must be defined if running in RetroArch.
The Saturn BIOS must be placed there, $sysdir/{sega_101,mpr-17933}.bin for Japanese, NA and EU regions respectively.

## Loading ISOs

Beetle Saturn needs a cue-sheet that points to an image file, usually an .iso/.bin file.
If you have e.g. <tt>foo.iso</tt>, you should create a foo.cue, and fill this in:

    FILE "foo.iso" BINARY
       TRACK 01 MODE1/2352
          INDEX 01 00:00:00

After that, you can load the <tt>foo.cue</tt> file as a ROM.
Note that this is a dirty hack and will not work on all games.
Ideally, make sure to use rips that have cue-sheets.

If foo is a multiple-disk game, you should have .cue files for each one, e.g. <tt>foo (Disc 1).cue</tt>, <tt>foo (Disc 2).cue</tt>, <tt>foo (Disc 3).cue</tt>.To take advantage of Beetle's Disk Control feature for disk swapping, an index file should be made.

Open a text file and enter your game's .cue files on it, like this:

    foo (Disc 1).cue
    foo (Disc 2).cue
    foo (Disc 3).cue

Save as foo.m3u and use this file in place of each disk's individual cue sheet.

## Suggested Firmware

- sega_101.bin (85ec9ca47d8f6807718151cbcca8b964)
- mpr-17933.bin (3240872c70984b6cbfda1586cab68dbe)
- mpr-18811-mx.ic1 (255113ba943c92a54facd25a10fd780c)
- mpr-19367-mx.ic1 (1cd19988d1d72a3e7caa0b73234c96b4)

## Options

* CD Image Cache - Loads the complete image in memory at startup
* Initial Scanline - Sets the first scanline to be drawn on screen
* Initial Scanline PAL - Sets the first scanline to be drawn on screen for PAL systems
* Last Scanline - Sets the last scanline to be drawn on screen
* Last Scanline PAL - Sets the last scanline to be drawn on screen for PAL systems
