#!/usr/bin/env python3
"""Compare Fort Candle benchmark results (`GameDevSS21 --benchmark ...`).

Usage:
    python tools/compare_benchmarks.py BASELINE CANDIDATE [--stat median]

BASELINE and CANDIDATE are result files or folders of result files. With several runs per side,
each value is the median over the runs, and "spread" shows how far the runs of a side scatter
(max - min of the per-run values, relative to that median). Differences smaller than the spread are noise.

Only the Python standard library is used.
"""

import argparse
import json
import statistics
import sys
from pathlib import Path

STATS = ("mean", "median", "p95", "p99", "max")

# Settings that must match for a fair comparison
SETUP_KEYS = ("scenario", "seed", "warmup_ticks", "ticks")


def load_runs(path):
    """All results from a file or a folder, as (path, data) pairs."""
    path = Path(path)
    files = sorted(path.glob("*.json")) if path.is_dir() else [path]
    runs = []
    for file in files:
        with open(file, encoding="utf-8") as f:
            data = json.load(f)
        if data.get("format") != "fort-candle-benchmark":
            sys.exit(f"{file}: not a Fort Candle benchmark result")
        if data.get("version") not in (2, 3):
            sys.exit(f"{file}: unsupported result version {data.get('version')}")
        runs.append((file, data))
    if not runs:
        sys.exit(f"{path}: no benchmark results found")
    return runs


def describe(runs):
    """One line about the builds of a side."""
    builds = {(d["build"]["time"], d["build"]["compiler"], d["build"]["config"]) for _, d in runs}
    parts = [f"built {time}, {compiler} {config}" for time, compiler, config in sorted(builds)]
    return f"{len(runs)} run(s): " + "; ".join(parts)


def check_setup(baseline, candidate):
    """Warnings about runs that should not be compared, or only with care."""
    warnings = []
    for key in SETUP_KEYS:
        values = {d[key] for _, d in baseline + candidate}
        if len(values) > 1:
            warnings.append(f"runs differ in {key}: {sorted(values, key=str)}")
    for key in ("renderer", "window"):
        values = {json.dumps(d[key]) for _, d in baseline + candidate}
        if len(values) > 1:
            warnings.append(f"runs differ in {key}: {sorted(values)} (frame and render times are not comparable)")
    configs = {d["build"]["config"] for _, d in baseline + candidate}
    if len(configs) > 1:
        warnings.append(f"runs differ in build config: {sorted(configs)}")
    if any(d["build"]["config"] == "Debug" for _, d in baseline + candidate):
        warnings.append("Debug builds are much slower and not representative")
    return warnings


def aggregate(runs, value_of):
    """Median over runs of value_of(data) and the relative spread, or None if no run has the value."""
    values = [v for v in (value_of(d) for _, d in runs) if v is not None]
    if not values:
        return None, None
    center = statistics.median(values)
    spread = (max(values) - min(values)) / center if len(values) > 1 and center else None
    return center, spread


def channel_stat(name, stat):
    def value_of(data):
        channel = data["channels_ms"].get(name, {})
        return channel.get(stat) if channel.get("samples", 0) > 0 else None

    return value_of


def fmt_ms(value):
    return "-" if value is None else f"{value:.4f}"


def fmt_delta(base, cand):
    if base is None or cand is None or base == 0:
        return "-"
    return f"{(cand - base) / base * 100:+.1f}%"


def fmt_spread(base_spread, cand_spread):
    spreads = [s for s in (base_spread, cand_spread) if s is not None]
    return f"{max(spreads) * 100:.1f}%" if spreads else "-"


def main():
    parser = argparse.ArgumentParser(description="Compare Fort Candle benchmark results.")
    parser.add_argument("baseline", help="result file or folder of result files")
    parser.add_argument("candidate", help="result file or folder of result files")
    parser.add_argument("--stat", choices=STATS, default="median", help="statistic to compare (default: median)")
    args = parser.parse_args()

    baseline = load_runs(args.baseline)
    candidate = load_runs(args.candidate)

    first = baseline[0][1]
    print(f"Scenario '{first['scenario']}', {first['ticks']} ticks after {first['warmup_ticks']} warm-up, seed {first['seed']}")
    print(f"  baseline:  {describe(baseline)}")
    print(f"  candidate: {describe(candidate)}")
    for warning in check_setup(baseline, candidate):
        print(f"  WARNING: {warning}")
    print()

    channels = list(first["channels_ms"].keys())
    for _, data in baseline + candidate:
        channels += [c for c in data["channels_ms"] if c not in channels]

    header = f"{'ms (' + args.stat + ')':<14} {'baseline':>10} {'candidate':>10} {'change':>9} {'spread':>8}"
    print(header)
    print("-" * len(header))
    for name in channels:
        base, base_spread = aggregate(baseline, channel_stat(name, args.stat))
        cand, cand_spread = aggregate(candidate, channel_stat(name, args.stat))
        print(f"{name:<14} {fmt_ms(base):>10} {fmt_ms(cand):>10} {fmt_delta(base, cand):>9} {fmt_spread(base_spread, cand_spread):>8}")

    print()
    base, base_spread = aggregate(baseline, lambda d: d["startup_ms"]["total"])
    cand, cand_spread = aggregate(candidate, lambda d: d["startup_ms"]["total"])
    print(f"{'startup ms':<14} {base:>10.0f} {cand:>10.0f} {fmt_delta(base, cand):>9} {fmt_spread(base_spread, cand_spread):>8}")
    base, _ = aggregate(baseline, lambda d: d["instances"]["max"])
    cand, _ = aggregate(candidate, lambda d: d["instances"]["max"])
    print(f"{'instances max':<14} {base:>10.0f} {cand:>10.0f}")


if __name__ == "__main__":
    main()
