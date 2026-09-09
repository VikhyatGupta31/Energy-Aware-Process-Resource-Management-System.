# energy_estimator.py
# Owner: Ansh
# Implements: E ~= U_cpu * t * C_cpu + IO_disk * C_disk + U_mem * t * C_mem
# Coefficients are literature-based approximations, NOT hardware measurements.
# This matches slide 5 of the Phase-I PPT exactly.

# ---------- Coefficients (placeholders — cite a source in your report) ----------
C_CPU = 0.90   # energy units per (% CPU utilization * second)
C_DISK = 0.02  # energy units per disk sector of I/O
C_MEM = 0.30   # energy units per (% memory utilization * second)


def estimate_energy(cpu_percent: float, mem_percent: float, disk_io_sectors: float, time_seconds: float) -> float:
    """
    Estimate energy consumption for one time window.
    cpu_percent, mem_percent: 0-100 values from Vikhyat's ResourceMonitor
    disk_io_sectors: cumulative disk I/O sectors for the window
    time_seconds: length of the sampling window
    """
    cpu_component = (cpu_percent / 100.0) * time_seconds * C_CPU
    disk_component = disk_io_sectors * C_DISK
    mem_component = (mem_percent / 100.0) * time_seconds * C_MEM
    return cpu_component + disk_component + mem_component


def estimate_energy_for_schedule(processes_with_metrics, time_per_unit=1.0):
    """
    Given a list of dicts like:
      {"pid": 1, "waiting_time": 4, "turnaround_time": 9, "cpu_percent": 60, "mem_percent": 20, "disk_io": 500}
    returns total estimated energy for the whole schedule.
    Useful for comparing FCFS vs SJF vs RR vs Priority vs Energy-Aware.
    """
    total = 0.0
    for p in processes_with_metrics:
        total += estimate_energy(
            cpu_percent=p["cpu_percent"],
            mem_percent=p["mem_percent"],
            disk_io_sectors=p["disk_io"],
            time_seconds=p["turnaround_time"] * time_per_unit,
        )
    return total


if __name__ == "__main__":
    print("=== Ansh: Energy Estimator Test ===\n")

    # Test 1: single sample (values like what Vikhyat's CSV would contain)
    e1 = estimate_energy(cpu_percent=45.0, mem_percent=30.0, disk_io_sectors=1200, time_seconds=1.0)
    print(f"Single sample energy estimate: {e1:.4f} units")

    # Test 2: whole-schedule comparison using dummy scheduler output
    fcfs_result = [
        {"pid": 1, "turnaround_time": 5, "cpu_percent": 60, "mem_percent": 25, "disk_io": 300},
        {"pid": 2, "turnaround_time": 8, "cpu_percent": 55, "mem_percent": 20, "disk_io": 150},
        {"pid": 3, "turnaround_time": 16, "cpu_percent": 70, "mem_percent": 35, "disk_io": 900},
    ]
    sjf_result = [
        {"pid": 1, "turnaround_time": 5, "cpu_percent": 60, "mem_percent": 25, "disk_io": 300},
        {"pid": 3, "turnaround_time": 13, "cpu_percent": 65, "mem_percent": 30, "disk_io": 700},
        {"pid": 2, "turnaround_time": 10, "cpu_percent": 50, "mem_percent": 18, "disk_io": 120},
    ]

    fcfs_energy = estimate_energy_for_schedule(fcfs_result)
    sjf_energy = estimate_energy_for_schedule(sjf_result)

    print(f"\nFCFS total estimated energy: {fcfs_energy:.4f} units")
    print(f"SJF  total estimated energy: {sjf_energy:.4f} units")
    print(f"\n{'SJF is more energy-efficient' if sjf_energy < fcfs_energy else 'FCFS is more energy-efficient'} in this test case.")
