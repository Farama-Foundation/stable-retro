#! /bin/bash
#
# picodrive release build script
#
# creates builds for the supported platforms in the release directory
#
# usage: release.sh <version> [platform...]
#	platforms:	gph dingux retrofw gcw0 rg350 psp
#
# expects toolchains to be installed in $HOME/opt:
#	gph:		arm-open2x-linux; arm-none-eabi 4.7 (from launchpad)
# 			needs additional libpng, ATM in src/gp2x/armroot/lib
#	dingux:		opendingux-toolchain (the old 2012 version)
#	retrofw:	mipsel-linux-uclibc (the retrofw toolchain)
#	gcw0:		gcw0-toolchain; mips-toolchain (buildroot, newer gcc)
#	rg350:		rg350-toolchain; mips-toolchain (buildroot, newer gcc)
#	miyoo:		miyoo
#	psp:		pspdev
# additionally needed libs for toolchains in $HOME/opt/lib and $HOME/opt/lib64

trap "exit" ERR

rel=$1
mkdir -p release-$rel

shift; plat=" $* "
[ -z "$plat" ] && plat=" gph dingux retrofw gcw0 rg350 psp "

[ -n "$LD_LIBRARY_PATH" ] && LD_LIBRARY_PATH=$LD_LIBRARY_PATH:
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}/usr/i686-linux-gnu/lib:$HOME/opt/lib
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/x86_64-linux-gnu/lib:$HOME/opt/lib64
export LD_LIBRARY_PATH

[ -z "${plat##* gph *}" ] && {
# GPH devices: gp2x, wiz, caanoo, with ubuntu arm gcc 4.7
# NB: -msoft-float uses the fpu setting for determining the parameter passing;
#     default upto gcc 4.7 was -mfpu=fpa, which has been removed in gcc 4.8, so
#     nothing newer than gcc 4.7 can be used here :-/
# NB: the arm-none-eabi toolchain is available for gcc 4.7, but it creates bad
#     ELF files for linux. The -Wl,-Ttext-segment=... below seems to fix this
TC=$HOME/opt/open2x/gcc-4.1.1-glibc-2.3.6 PATH=$HOME/opt/gcc-arm-none-eabi-4_7-2014q2/bin:$PATH CROSS_COMPILE=arm-none-eabi- CFLAGS="-I$TC/arm-open2x-linux/include -I$HOME/src/gp2x/armroot/include -U_FORTIFY_SOURCE -D__linux__" LDFLAGS="-B$TC/lib/gcc/arm-open2x-linux/4.1.1 -B$TC/arm-open2x-linux/lib -L$TC/arm-open2x-linux/lib -L$HOME/src/gp2x/armroot/lib -Wl,-Ttext-segment=0x10100" ./configure --platform=gp2x
PATH=$HOME/opt/gcc-arm-none-eabi-4_7-2014q2/bin:$PATH make clean all
PATH=$HOME/opt/gcc-arm-none-eabi-4_7-2014q2/bin:$PATH make -C platform/gp2x rel VER=$rel
mv PicoDrive_$rel.zip release-$rel/PicoDrive-gph_$rel.zip
}

[ -z "${plat##* dingux *}" ] && {
# dingux: dingoo a320, ritmix rzx-50, JZ4755 or older (mips32r1 w/o fpu)
# NB works for legacy dingux and possibly opendingux before gcw0
TC=$HOME/opt/opendingux-toolchain PATH=$TC/usr/bin:$PATH CROSS_COMPILE=mipsel-linux- CFLAGS="-I $TC/usr/include -I $TC/usr/include/SDL -Wno-unused-result -mabicalls" LDFLAGS="--sysroot $TC" ./configure --platform=dingux
TC=$HOME/opt/opendingux-toolchain PATH=$TC/usr/bin:$PATH make clean all
mv PicoDrive-dge.zip release-$rel/PicoDrive-dge_$rel.zip
}

[ -z "${plat##* retrofw *}" ] && {
# retrofw: rs-97 and similar, JZ4760 (mips32r1 with fpu)
TC=$HOME/opt/mipsel-linux-uclibc PATH=$TC/bin:$PATH CROSS_COMPILE=mipsel-linux- CFLAGS="-I $TC/mipsel-buildroot-linux-uclibc/sysroot/usr/include -I $TC/mipsel-buildroot-linux-uclibc/sysroot/usr/include/SDL -Wno-unused-result" LDFLAGS="--sysroot $TC/mipsel-buildroot-linux-uclibc/sysroot" ./configure --platform=retrofw
TC=$HOME/opt/mipsel-linux-uclibc PATH=$TC/bin:$PATH make clean all
mv PicoDrive.opk release-$rel/PicoDrive-retrofw_$rel.opk
}

[ -z "${plat##* gcw0 *}" ] && {
# gcw0: JZ4770 (mips32r2 with fpu), swapped X/Y buttons, SDK toolchain
#TC=$HOME/opt/gcw0-toolchain PATH=$TC/usr/bin:$PATH CROSS_COMPILE=mipsel-linux- TS=$TC/usr/mipsel-gcw0-linux-uclibc/sysroot CFLAGS="-I$TS/usr/include -I$TS/usr/include/SDL" LDFLAGS="--sysroot $TS" ./configure --platform=gcw0
#TC=$HOME/opt/gcw0-toolchain PATH=$TC/usr/bin:$PATH make clean all

# gcw0: JZ4770 (mips32r2 with fpu), swapped X/Y buttons, newer generic gcc
TC=$HOME/opt/gcw0-toolchain PATH=$HOME/opt/mips-toolchain/bin:$PATH CROSS_COMPILE=mipsel-linux- TS=$TC/usr/mipsel-gcw0-linux-uclibc/sysroot CFLAGS="-I$TS/usr/include -I$TS/usr/include/SDL -mabicalls" LDFLAGS="--sysroot $TS -Wl,--dynamic-linker=/lib/ld-uClibc.so.0" ./configure --platform=gcw0
PATH=$HOME/opt/mips-toolchain/usr/bin:$PATH make clean all
mv PicoDrive.opk release-$rel/PicoDrive-gcw0_$rel.opk
}

[ -z "${plat##* rg350 *}" ] && {
# rg350: JZ4770, SDK toolchain
#TC=$HOME/opt/rg350-toolchain PATH=$TC/usr/bin:$PATH CROSS_COMPILE=mipsel-linux- TS=$TC/usr/mipsel-gcw0-linux-uclibc/sysroot CFLAGS="-I$TS/usr/include -I$TS/usr/include/SDL" LDFLAGS="--sysroot $TS" ./configure --platform=rg350
#TC=$HOME/opt/rg350-toolchain PATH=$TC/usr/bin:$PATH make clean all

# rg350, gkd350h etc: JZ4770 or newer, newer generic gcc
# NB this may or may not work on the newer opendingux beta. undefine frame_info
# symbols to force linking them from libgcc.a - they might not be in the libc
TC=$HOME/opt/rg350-toolchain PATH=$HOME/opt/mips-toolchain/bin:$PATH CROSS_COMPILE=mipsel-linux- TS=$TC/usr/mipsel-gcw0-linux-uclibc/sysroot CFLAGS="-I$TS/usr/include -I$TS/usr/include/SDL -mabicalls" LDFLAGS="--sysroot $TS -Wl,--dynamic-linker=/lib/ld-uClibc.so.0 -u__register_frame_info -u__deregister_frame_info" ./configure --platform=opendingux
PATH=$HOME/opt/mips-toolchain/usr/bin:$PATH make clean all
mv PicoDrive.opk release-$rel/PicoDrive-opendingux_$rel.opk
}

[ -z "${plat##* miyoo *}" ] && {
# miyoo: BittBoy >=v1, PocketGo, Powkiddy [QV]90/Q20 (Allwinner F1C100s, ARM926)
TC=$HOME/opt/miyoo PATH=$TC/bin:$PATH CROSS_COMPILE=arm-miyoo-linux-uclibcgnueabi- CFLAGS="-I $TC/arm-miyoo-linux-uclibcgnueabi/sysroot/usr/include -I $TC/arm-miyoo-linux-uclibcgnueabi/sysroot/usr/include/SDL -Wno-unused-result" LDFLAGS="--sysroot $TC/arm-miyoo-linux-uclibcgnueabi/sysroot" ./configure --platform=miyoo
TC=$HOME/opt/miyoo PATH=$TC/bin:$PATH make clean all
mv PicoDrive.zip release-$rel/PicoDrive-miyoo_$rel.zip
}

[ -z "${plat##* psp *}" ] && {
# psp (experimental), pspdev SDK toolchain
TC=$HOME/opt/pspdev PATH=$TC/bin:$PATH CROSS_COMPILE=psp- CFLAGS="-I$TC/psp/sdk/include -D_POSIX_C_SOURCE=199506L" LDFLAGS="-L$TC/psp/sdk/lib" ./configure --platform=psp
TC=$HOME/opt/pspdev PATH=$TC/bin:$PATH make clean all
make -C platform/psp rel VER=$rel
mv PicoDrive_psp_$rel.zip release-$rel/PicoDrive-psp_$rel.zip
}
