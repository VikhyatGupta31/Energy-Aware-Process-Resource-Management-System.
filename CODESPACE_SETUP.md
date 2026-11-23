# Codespaces setup

## Python packages

Install the Python packages from the project root:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

`requirements.txt` lists Flask for the dashboard and `psutil` for live CPU, memory, process, and disk-throughput monitoring. `mysql-connector-python` is only needed if you configure the project to use MySQL. The default SQLite backend uses Python's built-in `sqlite3` module. Other Python modules used by this project are part of Python and do not need separate installation.

## System tool

The scheduling pipeline compiles `cpp/scheduler.cpp` as C++17. Codespaces needs `g++` available:

```bash
g++ --version
```

If `g++` is unavailable in the Codespace, install it with:

```bash
sudo apt-get update
sudo apt-get install -y g++
```

## Required project files

Keep these paths and files in the repository:

```text
cpp/scheduler.cpp
data/workload.csv
workload_generator.py
simulator.py
templates/index.html
app.py
db_loader.py
run_pipeline.py
requirements.txt
```

`data/workload.csv` is kept as the original example. The pipeline creates four repeatable workload scenarios through `workload_generator.py` and runs all five policies on each one. The dashboard can animate a scheduling simulation and separately refresh live host telemetry every two seconds. Scheduling is not applied to real operating-system processes.

Workload CSV files use this header:

```text
pid,arrival_time,burst_time,priority
```

## Run the pipeline and dashboard

From the repository root:

```bash
source .venv/bin/activate
python run_pipeline.py
python -m flask --app app run --host 0.0.0.0 --port 5000
```

Leave the dashboard command running. In Codespaces, open the **Ports** tab and open forwarded port 5000 in the browser.

## Database mode

SQLite is the default and does not need a separate database server. To use MySQL, install and configure a MySQL server, set `DB_BACKEND=mysql` and the `MYSQL_*` environment variables, then run the pipeline. Never commit a real database password.
