#!/usr/bin/env python3
"""Benchmark max emulation throughput per libretro core (single-threaded).

This script tries to find at least one installed ROM per system, then benchmarks
one representative system per unique libretro core library.

It reports steps/second (roughly FPS when the core runs one frame per step).

Notes:
- Many cores will internally use multiple threads; this script only ensures the
  Python harness is single-threaded and sets common thread-limit env vars.
- If you enable `--screen`, the benchmark will include CPU framebuffer capture
  overhead (potentially very large for GPU-rendered paths with readback).
"""

from __future__ import annotations

import argparse
import gc
import os
import sys
import time
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Tuple


def _set_single_thread_env() -> None:
    # Best-effort: common threadpool env vars.
    os.environ.setdefault("OMP_NUM_THREADS", "1")
    os.environ.setdefault("OPENBLAS_NUM_THREADS", "1")
    os.environ.setdefault("MKL_NUM_THREADS", "1")
    os.environ.setdefault("VECLIB_MAXIMUM_THREADS", "1")
    os.environ.setdefault("NUMEXPR_NUM_THREADS", "1")


def _parse_integrations(s: str):
    import stable_retro.data as d

    s = s.strip().lower()
    if s in {"default"}:
        return d.Integrations.DEFAULT
    if s in {"stable"}:
        return d.Integrations.STABLE
    if s in {"contrib"}:
        return d.Integrations.CONTRIB_ONLY
    if s in {"experimental"}:
        return d.Integrations.EXPERIMENTAL_ONLY
    if s in {"custom"}:
        return d.Integrations.CUSTOM_ONLY
    if s in {"all"}:
        return d.Integrations.ALL
    raise SystemExit(f"Unknown integrations: {s} (use one of: default|stable|contrib|experimental|custom|all)")


@dataclass(frozen=True)
class Candidate:
    system: str
    game: str
    rom_path: str


@dataclass(frozen=True)
class CoreBenchResult:
    core_lib: str
    system: str
    game: str
    seconds: float
    steps: int
    steps_per_sec: float
    screen: bool


@dataclass(frozen=True)
class CoreSkip:
    core_lib: str
    reason: str


def _iter_installed_roms(inttype) -> List[Candidate]:
    import stable_retro
    import stable_retro.data as d

    out: List[Candidate] = []
    for game in d.list_games(inttype):
        try:
            rom = d.get_romfile_path(game, inttype)
        except Exception:
            continue
        try:
            system = stable_retro.get_romfile_system(rom)
        except Exception:
            continue
        out.append(Candidate(system=system, game=game, rom_path=rom))
    return out


def _pick_representative_rom_per_system(cands: Iterable[Candidate]) -> Dict[str, Candidate]:
    chosen: Dict[str, Candidate] = {}
    for c in cands:
        if c.system not in chosen:
            chosen[c.system] = c
    return chosen


def _group_systems_by_core_lib() -> Dict[str, List[str]]:
    import stable_retro.data as d

    lib_to_systems: Dict[str, List[str]] = {}
    for system, info in d.EMU_INFO.items():
        lib = info.get("lib")
        if not lib:
            continue
        lib_to_systems.setdefault(lib, []).append(system)

    # Stable ordering for output.
    for lib in list(lib_to_systems.keys()):
        lib_to_systems[lib] = sorted(set(lib_to_systems[lib]))
    return dict(sorted(lib_to_systems.items(), key=lambda kv: kv[0]))


def _bench_one_rom(
    rom_path: str,
    seconds: float,
    screen: bool,
) -> Tuple[int, float]:
    import stable_retro
    import stable_retro.data as d

    # Ensure only one emulator exists per process.
    gc.collect()

    data = d.GameData()  # empty is fine for stepping
    em = stable_retro.RetroEmulator(rom_path)
    try:
        em.configure_data(data)
        em.step()  # initialize

        # Warmup
        for _ in range(30):
            em.step()
            if screen:
                _ = em.get_screen()

        start = time.perf_counter()
        steps = 0
        while True:
            now = time.perf_counter()
            if now - start >= seconds:
                break
            em.step()
            steps += 1
            if screen:
                _ = em.get_screen()

        elapsed = max(1e-9, time.perf_counter() - start)
        return steps, elapsed
    finally:
        # Be explicit; stable-retro historically enforces one emulator at a time.
        try:
            del em
        except Exception:
            pass
        gc.collect()


def _fmt_float(x: float) -> str:
    if x >= 1000:
        return f"{x:,.0f}"
    if x >= 100:
        return f"{x:,.1f}"
    return f"{x:,.2f}"


def main(argv: Optional[List[str]] = None) -> int:
    _set_single_thread_env()

    p = argparse.ArgumentParser(description="Benchmark max FPS (steps/sec) per libretro core.")
    p.add_argument("--seconds", type=float, default=5.0, help="Benchmark duration per core (default: 5)")
    p.add_argument(
        "--integrations",
        type=str,
        default="all",
        help="Which integrations to search for ROMs (default: all)",
    )
    p.add_argument(
        "--screen",
        action="store_true",
        help="Include `get_screen()` per step (measures capture/readback overhead)",
    )
    p.add_argument(
        "--hw-render",
        action="store_true",
        help="Set `STABLE_RETRO_HW_RENDER=1` before running cores",
    )
    p.add_argument(
        "--n64-gfxplugin",
        type=str,
        default=None,
        help="If set, exports `STABLE_RETRO_PARALLEL_N64_GFXPLUGIN` (e.g. glide64|gln64|rice|angrylion)",
    )
    args = p.parse_args(argv)

    if args.seconds <= 0:
        raise SystemExit("--seconds must be > 0")

    if args.hw_render:
        os.environ["STABLE_RETRO_HW_RENDER"] = "1"
    if args.n64_gfxplugin:
        os.environ["STABLE_RETRO_PARALLEL_N64_GFXPLUGIN"] = args.n64_gfxplugin

    # Import after env is set.
    import stable_retro.data as d

    inttype = _parse_integrations(args.integrations)

    cands = _iter_installed_roms(inttype)
    by_system = _pick_representative_rom_per_system(cands)

    lib_to_systems = _group_systems_by_core_lib()

    results: List[CoreBenchResult] = []
    skips: List[CoreSkip] = []

    interrupted = False
    for lib, systems in lib_to_systems.items():
        chosen: Optional[Candidate] = None
        for sysname in systems:
            c = by_system.get(sysname)
            if c is not None:
                chosen = c
                break
        if chosen is None:
            skips.append(CoreSkip(core_lib=lib, reason="no ROM found for any supported system"))
            continue

        try:
            steps, elapsed = _bench_one_rom(chosen.rom_path, seconds=args.seconds, screen=args.screen)
            sps = steps / elapsed if elapsed > 0 else 0.0
            results.append(
                CoreBenchResult(
                    core_lib=lib,
                    system=chosen.system,
                    game=chosen.game,
                    seconds=elapsed,
                    steps=steps,
                    steps_per_sec=sps,
                    screen=args.screen,
                )
            )
        except KeyboardInterrupt:
            interrupted = True
            break
        except Exception as e:
            skips.append(CoreSkip(core_lib=lib, reason=f"error: {type(e).__name__}: {e}"))

    results_sorted = sorted(results, key=lambda r: r.steps_per_sec, reverse=True)

    print(f"ROM systems found: {len(by_system)} | Cores discovered: {len(lib_to_systems)}")
    print(f"Benchmark: {args.seconds}s per core | screen={args.screen}")
    if args.hw_render:
        print("Env: STABLE_RETRO_HW_RENDER=1")
    if args.n64_gfxplugin:
        print(f"Env: STABLE_RETRO_PARALLEL_N64_GFXPLUGIN={args.n64_gfxplugin}")
    print()

    if interrupted:
        print("Interrupted (Ctrl-C). Showing partial results.\n")

    if results_sorted:
        # Basic table. FPS here is equivalent to steps/sec (1 step ≈ 1 frame).
        # Use explicit separators so values can't appear "missing" due to padding.
        print("fps | core | system | game")
        print("--- | ---- | ------ | ----")
        for r in results_sorted:
            print(
                f"{_fmt_float(r.steps_per_sec)} | "
                f"{r.core_lib} | "
                f"{r.system} | "
                f"{r.game}"
            )
        print()

    if skips:
        print("Skipped:")
        for s in skips:
            print(f"- {s.core_lib}: {s.reason}")

    # Exit non-zero only if nothing ran.
    return 0 if results_sorted else 2


if __name__ == "__main__":
    raise SystemExit(main())
