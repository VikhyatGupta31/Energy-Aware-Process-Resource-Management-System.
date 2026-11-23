# Energy-Aware Process and Resource Management System

A runnable implementation of the Energy-Aware Process and Resource Management project. Each pipeline run creates four synthetic workload types with three seeded trials each, then runs FCFS, SJF, Round Robin, Priority, and Energy-Aware scheduling on every trial. All policies receive the same process list within a trial. It retains experiment results in SQLite, records a system resource snapshot per trial, benchmarks an indexed versus unindexed query, and serves a Flask dashboard with workload comparisons, an interactive CPU timeline, and a live system resource monitor.

The scheduler is a simulator. It does not change operating-system scheduling or execute the sample processes. The dashboard separately samples live host CPU, memory, process count, and disk throughput every two seconds. These readings describe the machine running Flask (in Codespaces, that is the Codespace container). Energy is a formula-based estimate, not a physical measurement. The default SQLite configuration works on Windows, macOS, and Linux. `schema.sql` and the optional MySQL loader support the report's MySQL design.

The energy-aware policy scores each ready process using estimated CPU cost plus deterministic priority / process-ID proxies for memory and I/O demand. Those proxies keep the policy runnable with the four-column workload; they are not measured per-process resource values.

The reported per-process energy estimate is:

```text
score = burst_time * (0.65 + 0.20 * 0.35) + turnaround_time * 0.15 * 0.25
```

Here `0.65`, `0.20`, and `0.15` are the CPU, memory, and I/O weights; `0.35` and `0.25` are fixed memory and I/O factors. Turnaround time is only a rough I/O-related proxy in this model. The score is in relative software units, not watts or kilowatt-hours, and is not a physical energy measurement. The dashboard displays this formula and its assumptions.

Workloads use fixed seeds so experiments can be repeated consistently. The four cases are `baseline`, `cpu_heavy`, `short_jobs`, and `staggered_arrivals`; each has three seeded trial variants. Every policy receives the same input list within a given trial. Each full pipeline run gets a run ID; previous scheduling and resource results are retained in the database for history.

The dashboard reports average waiting time, turnaround time, response time, throughput, and estimated energy score. Throughput is completed processes divided by elapsed simulation time. The Results page summarizes the three trial averages, shows the waiting-time range across trials, and marks the policy with the lowest waiting time, response time, and energy estimate for each workload. These marks describe separate goals; there may not be one policy that is best on every measure.

## Requirements

- Python 3.10 or newer
- `psutil` (installed from `requirements.txt`) for live cross-platform resource monitoring
- A C++17 compiler: `g++` (Linux/macOS/WSL) or Visual Studio C++ Build Tools (`cl`) on Windows
- MySQL is optional; SQLite is the default

## Run on Windows

Open PowerShell in this folder and run:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
python run_pipeline.py
python app.py
```

Open http://127.0.0.1:5000. If script activation is blocked, run `Set-ExecutionPolicy -Scope Process Bypass` in that PowerShell window and activate again. The pipeline compiles the C++ scheduler with `cl` when available, otherwise with `g++`.

## Run on Linux / macOS / WSL

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
g++ -std=c++17 -O2 cpp/scheduler.cpp -o scheduler
python run_pipeline.py
python app.py
```

The pipeline itself compiles the executable, so the manual `g++` command is optional. On Linux, `/proc` is used for CPU, memory, disk, and process snapshots. Windows uses system APIs for CPU, memory, and process count; disk I/O is reported as zero where a portable counter is unavailable.

## Workload scenarios

Run `python run_pipeline.py` to generate four workload types with three seeded trials each and compare all five policies. The generator is `workload_generator.py`. To change trial count, scenario sizes, or ranges, edit that generator. Lower numeric priority values mean higher priority.

Use the dashboard workload dropdown to view one scenario at a time. Results for all scenarios in the latest experiment are shown; earlier experiments remain stored in the database.

## Optional MySQL

Create a database and apply `schema.sql`, install `mysql-connector-python`, then set these environment variables before running the pipeline:

```powershell
$env:DB_BACKEND='mysql'
$env:MYSQL_HOST='127.0.0.1'
$env:MYSQL_PORT='3306'
$env:MYSQL_USER='root'
$env:MYSQL_PASSWORD='your-password'
$env:MYSQL_DATABASE='energy_aware_os'
python run_pipeline.py
```

The dashboard reads the database selected by `DB_BACKEND`. For MySQL, install the optional connector with `pip install mysql-connector-python`. Do not commit real credentials. SQLite data is stored in `data/project.db`.

## Outputs

- `data/scheduling_results.csv` - per-process metrics for all five policies
- `data/resource_logs.csv` - OS resource snapshot
- `data/workloads/` - four repeatable generated workload inputs
- `data/project.db` - local dashboard database (SQLite mode)
- `data/query_benchmark.csv` - indexed/unindexed benchmark summary

The dashboard's **Scheduler simulation** page animates CPU time slices and reports waiting, turnaround, and response time for a selected policy and workload. This is a discrete-event simulation; it is separate from the live host monitor.

The benchmark is illustrative: it creates a larger local table and compares equivalent lookups with and without an index. Timing varies by computer and database cache state.
