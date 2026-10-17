# Energy-Aware Process and Resource Management System

A small, runnable implementation of the Phase-I/Phase-II report. It runs the same synthetic workload through FCFS, SJF, Round Robin, Priority, and Energy-Aware scheduling; writes CSV results; records a resource snapshot and results in SQLite; benchmarks an indexed versus unindexed query; and serves a Flask dashboard.

The scheduler is a simulator. It does not change operating-system scheduling or measure electrical power. Energy is a formula-based estimate. The default SQLite configuration works on Windows, macOS, and Linux. `schema.sql` and the optional MySQL loader support the report's MySQL design.

The energy-aware policy scores each ready process using estimated CPU cost plus deterministic priority / process-ID proxies for memory and I/O demand. Those proxies keep the policy runnable with the report's four-column workload; they are not measured per-process resource values. Linux reads `/proc`; Windows reads system memory and process counters.

## Requirements

- Python 3.10 or newer
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

## Use a custom workload

Edit `data/workload.csv` (columns: `pid,arrival_time,burst_time,priority`) and rerun `python run_pipeline.py`. Lower numeric priority values mean higher priority. The default workload is deterministic and can be regenerated from `data/workload.csv.example` if needed.

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
- `data/project.db` - local dashboard database (SQLite mode)
- `data/query_benchmark.csv` - indexed/unindexed benchmark summary

The benchmark is illustrative: it creates a larger local table and compares equivalent lookups with and without an index. Timing varies by computer and database cache state.
