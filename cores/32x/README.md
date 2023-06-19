This is my foray into dynamic recompilation using PicoDrive, a
Megadrive / Genesis / Sega CD / Mega CD / 32X / SMS emulator.

I added support for MIPS (mips32r2), ARM64 (armv8), RISC-V (RV64IM) and
PowerPC (G4/2.03) support to the SH2 recompiler, as well as spent much effort to
optimize the DRC-generated code. I also optimized SH2 memory access inside the
emulator, and did some work on M68K/SH2 CPU synchronization to fix some problems
and speed up the emulator.

It got a bit out of hand. I ended up doing fixes and optimizations all over the
place, mainly for 32X and CD, graphics handling, sound, and probably some more,
see the commit history. As a result, 32X emulation speed has improved a lot, a
lot of bugs were fixed, and some new features (e.g. chd file support) have been
added.

### compiling

I mainly worked with standalone PicoDrive versions as created by configure/make.
A list of platforms for which this is possible can be obtained with

> configure --help

If you want to build an executable for a unixoid platform not listed in the
platform list, just use

> configure --platform=generic

If DRC is available for the platform, it should be enabled automatically.

For other platforms using a cross-compiling toolchain I used this,
assuming $TC points to the appropriate cross compile toolchain directory:

platform|toolchain|configure command
--------|---------|-----------------
gp2x,wiz,caanoo|open2x|CROSS_COMPILE=arm-open2x-linux- CFLAGS="-I$TC/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/include" LDFLAGS="--sysroot $TC/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux -L$TC/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/lib" ./configure --platform=gp2x
gp2x,wiz,caanoo|open2x with ubuntu arm gcc 4.7|CROSS_COMPILE=arm-linux-gnueabi- CFLAGS="-I$TC/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/include" LDFLAGS="-B$TC/gcc-4.1.1-glibc-2.3.6/lib/gcc/arm-open2x-linux/4.1.1 -B$TC/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/lib -L$TC/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/lib" ./configure --platform=gp2x
dingux|dingux|CROSS_COMPILE=mipsel-linux- CFLAGS="-I$TC/usr/include -I$TC/usr/include/SDL" LDFLAGS="--sysroot $TC -L$TC/lib" ./configure --platform=dingux
dingux|dingux with ubuntu mips gcc 5.4|CROSS_COMPILE=mipsel-linux-gnu- CFLAGS="-I$TC/usr/include -I$TC/usr/include/SDL" LDFLAGS="-B$TC/usr/lib -B$TC/lib -Wl,-rpath-link=$TC/usr/lib -Wl,-rpath-link=$TC/lib" ./configure --platform=dingux
retrofw|retrofw|CROSS_COMPILE=mipsel-linux- CFLAGS="-I $TC/include -I $TC/include/SDL -Wno-unused-result" LDFLAGS="--sysroot $TC/mipsel-buildroot-linux-uclibc/sysroot" ./configure --platform=retrofw
gcw0|gcw0|CROSS_COMPILE=mipsel-gcw0-linux-uclibc- CFLAGS="-I$TC/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/include -I$TC/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/include/SDL" LDFLAGS="--sysroot $TC/usr/mipsel-gcw0-linux-uclibc/sysroot" ./configure --platform=gcw0
rg350|rg350|CROSS_COMPILE=mipsel-linux- CFLAGS="-I$TC/usr/include -I$TC/usr/include/SDL" LDFLAGS="--sysroot $TC -L$TC/lib" ./configure --platform=rg350
miyoo|miyoo|CROSS_COMPILE=arm-miyoo-linux-uclibcgnueabi- CFLAGS="-I$TC/arm-miyoo-linux-uclibcgnueabi/sysroot/usr/include -I$TC/arm-miyoo-linux-uclibcgnueabi/sysroot/usr/include/SDL" LDFLAGS="--sysroot $TC/arm-miyoo-linux-uclibcgnueabi/sysroot" ./configure --platform=miyoo

For gp2x, wiz, and caanoo you may need to compile libpng first.

After configure, compile with

> make

### helix MP3 decoder for ARM

For 32 bit ARM platforms, there is the possibility to compile the helix MP3
decoder into a shared library to be able to use MP3 audio files with CD games.
The helix source files aren't supplied because of licensing issues. However, if
you have obtained the sources, put them into the platform/common/helix
directory, set CROSS to your cross compiler prefix (e.g. arm-linux-gnueabi-)
and LIBGCC to your cross compiler's libgcc.a
(e.g. /usr/lib/gcc-cross/arm-linux-gnueabi/4.7/libgcc.a), and compile with

> make -C platform/common/helix CROSS=$CROSS LIBGCC=$LIBGCC

Copy the resulting ${CROSS}helix_mp3.so as libhelix.so to the directory where
the PicoDrive binary is.

### installing

You need to install the resulting binary onto your device manually.
For opendingux, gcw0, and rg350, copy the opk to your SD card.
For gp2x, wiz, and caanoo, the easiest way is to unpack
[PicoDrive_191.zip](http://notaz.gp2x.de/releases/PicoDrive/PicoDrive_191.zip)
on your SD card and replace the PicoDrive binary.
For legacy dingux, unpack the dge zip and copy the files to your SD card.

Send bug reports, fixes etc to <derkub@gmail.com>
