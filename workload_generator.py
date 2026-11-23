"""Create repeatable synthetic workloads for scheduler comparison."""
import csv
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "data" / "workloads"
SEED = 2026
REPETITIONS = 3


def write_workload(name, processes):
    path = OUT / f"{name}.csv"
    with path.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["pid", "arrival_time", "burst_time", "priority"])
        writer.writerows(processes)
    return path


def generate():
    rng = random.Random(SEED)
    OUT.mkdir(parents=True, exist_ok=True)

    # A small baseline retained for easy comparison with the original demo.
    baseline = [
        (1, 0, 7, 2), (2, 2, 4, 1), (3, 3, 1, 3),
        (4, 5, 4, 2), (5, 6, 6, 1), (6, 8, 3, 4),
    ]
    write_workload("baseline", baseline)

    # Many long CPU bursts stress policies that favor short jobs.
    cpu_heavy = [
        (100 + pid, rng.randint(0, 12), rng.randint(12, 30), rng.randint(1, 5))
        for pid in range(1, 21)
    ]
    write_workload("cpu_heavy", cpu_heavy)

    # Many short jobs make queue ordering and scheduling overhead visible.
    short_jobs = [
        (200 + pid, rng.randint(0, 18), rng.randint(1, 5), rng.randint(1, 5))
        for pid in range(1, 31)
    ]
    write_workload("short_jobs", short_jobs)

    # Arrival times spread out to exercise idle periods and changing ready queues.
    staggered = [
        (300 + pid, (pid - 1) * 3, rng.randint(2, 14), rng.randint(1, 5))
        for pid in range(1, 21)
    ]
    write_workload("staggered_arrivals", staggered)
    return sorted(OUT.glob("*.csv"))


def generate_trials(repetitions=REPETITIONS):
    """Create seeded variations so each policy is compared on matching trials."""
    trial_dir = OUT / "trials"
    trial_dir.mkdir(parents=True, exist_ok=True)
    generated = []
    for trial in range(1, repetitions + 1):
        rng = random.Random(SEED + trial)
        scenarios = {
            "baseline": [
                (trial * 1000 + pid, rng.randint(0, 8), rng.randint(1, 10), rng.randint(1, 5))
                for pid in range(1, 7)
            ],
            "cpu_heavy": [
                (trial * 1000 + 100 + pid, rng.randint(0, 12), rng.randint(12, 30), rng.randint(1, 5))
                for pid in range(1, 21)
            ],
            "short_jobs": [
                (trial * 1000 + 200 + pid, rng.randint(0, 18), rng.randint(1, 5), rng.randint(1, 5))
                for pid in range(1, 31)
            ],
            "staggered_arrivals": [
                (trial * 1000 + 300 + pid, (pid - 1) * 3, rng.randint(2, 14), rng.randint(1, 5))
                for pid in range(1, 21)
            ],
        }
        for name, processes in scenarios.items():
            path = trial_dir / f"{name}_trial_{trial:02d}.csv"
            with path.open("w", newline="", encoding="utf-8") as file:
                writer = csv.writer(file)
                writer.writerow(["pid", "arrival_time", "burst_time", "priority"])
                writer.writerows(processes)
            generated.append((name, trial, path))
    return generated


if __name__ == "__main__":
    for workload_path in generate():
        print(f"Prepared {workload_path.name}")
