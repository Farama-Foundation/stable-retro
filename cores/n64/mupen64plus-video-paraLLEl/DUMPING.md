# How to debug RDP dumps

## Build

Make sure to build parallel-n64 with `make HAVE_PARALLEL=1 HAVE_PARALLEL_RSP=1 HAVE_RDP_DUMP=1`.
Build `git://github.com/Themaister/parallel-rdp` as instructed in the repo. You should have a `rdp-validate-dump` binary.

## Workflow

- Find place in game that shows a difference with Angrylion vs parallel-RDP.
- Make savestate close to where issue happens.
- Switch plugin to Angrylion.
- Exit RetroArch
- Set environment variable `RDP_DUMP` to something like `dump.rdp`. E.g. `RDP_DUMP=dump.rdp ./retroarch`.
- Run game, it should run quite slow, as multithreading is forced off when dumping.
- You should now have a dump where you pointed `RDP_DUMP` to.
- Run `rdp-validate-dump dump.rdp`. If it passes without error, draw calls are bitexact with Angrylion.
- Also to be sure, run `rdp-validate-dump dump.rdp --sync-only`. This catches batching-related bugs. This should run much faster.
- If both pass without error, you can be confident parallel-RDP was bitexact. If an error is found, it should be reported with the dump.
