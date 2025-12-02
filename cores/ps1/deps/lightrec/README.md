
# Lightrec

Lightrec is a MIPS-to-everything dynamic recompiler for
PlayStation emulators, using
[GNU Lightning](https://www.gnu.org/software/lightning/)
as the code emitter.

As such, in theory it should be able to run on every CPU that Lightning
can generate code for; including, but not limited to, __x86__, __x86_64__,
__ARM__, __Aarch64__, __MIPS__, __PowerPC__, __SH4__ and __Risc-V__.

## Features

* __High-level optimizations__.  The MIPS code is first pre-compiled into
a form of Intermediate Representation (IR).
Basically, just a single-linked list of structures representing the
instructions. On that list, several optimization steps are performed:
instructions are modified, reordered, tagged; new meta-instructions
can also be added.

* __Lazy compilation__.
If Lightrec detects a block of code that would be very hard to
compile properly (e.g. a branch with a branch in its delay slot),
the block is marked as not compilable, and will always be emulated
with the built-in interpreter. This allows to keep the code emitter
simple and easy to understand.

* __Run-time profiling__.
The generated code will gather run-time information about the I/O access
(whether they hit RAM, or hardware registers).
The code generator will then use this information to generate direct
read/writes to the emulated memories, instead of jumping to C for
every call.

* __Threaded compilation__.
When entering a loading zone, where a lot of code has to be compiled,
we don't want the compilation process to slow down the pace of emulation.
To avoid that, the code compiler optionally runs on a thread, and the
main loop will emulate the blocks that have not been compiled yet with
the interpreter. This helps to drastically reduce the stutter that
typically happens when a lot of new code is run.

## Emulators

Lightrec has been ported to the following emulators:

* [__PCSX-ReArmed__ (libretro)](https://github.com/libretro/pcsx_rearmed)

* [__Beetle__ (libretro)](https://github.com/libretro/beetle-psx-libretro/)

* [__CubeSX/WiiSX__](https://github.com/emukidid/pcsxgc/)

* [__Bloom__](https://github.com/pcercuei/bloom)

[![Star History Chart](https://api.star-history.com/svg?repos=pcercuei/lightrec&type=Date)](https://star-history.com/#pcercuei/lightrec&Date)
