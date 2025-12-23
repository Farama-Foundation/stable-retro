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
import json
import os
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


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
    raise SystemExit(
        f"Unknown integrations: {s} (use one of: default|stable|contrib|experimental|custom|all)",
    )


@dataclass(frozen=True)
class BenchmarkSpec:
    core_lib: str
    system: str
    game: str


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


def _default_benchmark_json_path() -> Path:
    # Default to this script's directory.
    return Path(__file__).resolve().with_name("benchmark.json")


def _load_benchmark_specs(path: Path) -> list[BenchmarkSpec]:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as e:
        raise SystemExit(
            f"Benchmark file not found: {path} (create it or pass --benchmark-json)",
        ) from e

    if not isinstance(raw, dict) or "benchmarks" not in raw:
        raise SystemExit(f"Invalid benchmark file format: {path}")

    benches = raw.get("benchmarks")
    if not isinstance(benches, list) or not benches:
        raise SystemExit(f"Benchmark file has no benchmarks: {path}")

    specs: list[BenchmarkSpec] = []
    for i, item in enumerate(benches):
        if not isinstance(item, dict):
            raise SystemExit(f"Invalid benchmark entry at index {i} in {path}")
        try:
            core_lib = str(item["core_lib"])
            system = str(item["system"])
            game = str(item["game"])
        except KeyError as e:
            raise SystemExit(
                f"Missing key {e} in benchmark entry at index {i} in {path}",
            ) from e
        specs.append(BenchmarkSpec(core_lib=core_lib, system=system, game=game))
    return specs


def _iter_installed_roms(inttype) -> list[Candidate]:
    import stable_retro
    import stable_retro.data as d

    out: list[Candidate] = []
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


def _pick_representative_rom_per_system(
    cands: Iterable[Candidate],
) -> dict[str, Candidate]:
    chosen: dict[str, Candidate] = {}
    for c in cands:
        if c.system not in chosen:
            chosen[c.system] = c
    return chosen


def _group_systems_by_core_lib() -> dict[str, list[str]]:
    import stable_retro.data as d

    lib_to_systems: dict[str, list[str]] = {}
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
    *,
    warmup_steps: int,
) -> tuple[int, float]:
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
        for _ in range(warmup_steps):
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


def _parse_args(argv: list[str] | None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Benchmark max FPS (steps/sec) per libretro core.",
    )
    p.add_argument(
        "--seconds",
        type=float,
        default=5.0,
        help="Benchmark duration per entry (default: 5)",
    )
    p.add_argument(
        "--warmup-steps",
        type=int,
        default=30,
        help="Warmup steps before timing (default: 30)",
    )
    p.add_argument(
        "--benchmark-json",
        type=str,
        default=str(_default_benchmark_json_path()),
        help="Benchmark spec JSON file (default: scripts/benchmark.json)",
    )
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
    if args.warmup_steps < 0:
        raise SystemExit("--warmup-steps must be >= 0")
    return args


def main(argv: list[str] | None = None) -> int:
    _set_single_thread_env()

    args = _parse_args(argv)

    if args.hw_render:
        os.environ["STABLE_RETRO_HW_RENDER"] = "1"
    if args.n64_gfxplugin:
        os.environ["STABLE_RETRO_PARALLEL_N64_GFXPLUGIN"] = args.n64_gfxplugin

    inttype = _parse_integrations(args.integrations)

    bench_path = Path(args.benchmark_json)
    specs = _load_benchmark_specs(bench_path)

    # Optional consistency check: map system -> core lib from EMU_INFO.
    import stable_retro.data as d

    system_to_lib: dict[str, str] = {}
    for system, info in d.EMU_INFO.items():
        lib = info.get("lib")
        if lib:
            system_to_lib[system] = lib

    results: list[CoreBenchResult] = []
    skips: list[CoreSkip] = []

    interrupted = False
    for spec in specs:
        expected_lib = system_to_lib.get(spec.system)
        if expected_lib is not None and expected_lib != spec.core_lib:
            skips.append(
                CoreSkip(
                    core_lib=spec.core_lib,
                    reason=(
                        f"system/core mismatch for {spec.system}: benchmark.json has {spec.core_lib}"
                        f" but EMU_INFO maps to {expected_lib}"
                    ),
                ),
            )
            continue

        try:
            rom_path = d.get_romfile_path(spec.game, inttype)
        except Exception as e:
            skips.append(
                CoreSkip(
                    core_lib=spec.core_lib,
                    reason=f"ROM not found for game {spec.game} ({spec.system}): {type(e).__name__}: {e}",
                ),
            )
            continue

        try:
            steps, elapsed = _bench_one_rom(
                rom_path,
                seconds=args.seconds,
                screen=args.screen,
                warmup_steps=args.warmup_steps,
            )
            sps = steps / elapsed if elapsed > 0 else 0.0
            results.append(
                CoreBenchResult(
                    core_lib=spec.core_lib,
                    system=spec.system,
                    game=spec.game,
                    seconds=elapsed,
                    steps=steps,
                    steps_per_sec=sps,
                    screen=args.screen,
                ),
            )
        except KeyboardInterrupt:
            interrupted = True
            break
        except Exception as e:
            skips.append(
                CoreSkip(
                    core_lib=spec.core_lib,
                    reason=f"error running {spec.game}: {type(e).__name__}: {e}",
                ),
            )

    results_sorted = sorted(results, key=lambda r: r.steps_per_sec, reverse=True)

    print(f"Benchmark spec: {bench_path}")
    print(
        f"Benchmark: {args.seconds}s per entry | warmup_steps={args.warmup_steps} | screen={args.screen}",
    )
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
                f"{r.game}",
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
