[![Build Status](https://travis-ci.org/libretro/parallel-n64.svg?branch=master)](https://travis-ci.org/libretro/parallel-n64)
[![Build status](https://ci.appveyor.com/api/projects/status/iqe836smfugoy8ey/branch/master?svg=true)](https://ci.appveyor.com/project/bparker06/parallel-n64/branch/master)

# mupen64plus-libretro

To enable a dynarec CPU core you must pass the WITH_DYNAREC value to make:
* make WITH_DYNAREC=x86
* make WITH_DYNAREC=x86_64
* make WITH_DYNAREC=arm
* make WITH_DYNAREC=aarch64

New make options:
* USE_CXD4_NEW - use the most recent version of CXD4 that was verified on Android
* USE_SSE2NEON - enable SSE2 vectorized routines on ARMv7+ via hacked SSE2NEON library

To build Android hardfp library with the new CXD4 RSP + NEON + Parallel RDP do:
* ndk-build -j8 USE_SSE2NEON=1 APP_ABI=armeabi-v7a-hard

To build Android arm64 library with the new CXD4 RSP + Parallel RDP + dynarec do:
* ndk-build APP_ABI=arm64-v8a
