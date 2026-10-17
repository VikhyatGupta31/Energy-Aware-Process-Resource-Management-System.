#!/usr/bin/env bash
set -euo pipefail

# Always run from this script's project directory.
cd "$(dirname "$(realpath "$0")")"

if ! command -v python3 >/dev/null 2>&1; then
  echo "ERROR: Python 3 is required. Install Python 3 in this Codespace."
  exit 1
fi

if ! command -v g++ >/dev/null 2>&1; then
  echo "ERROR: g++ is required to build the C++17 scheduler."
  echo "Install it with: sudo apt-get update && sudo apt-get install -y g++"
  exit 1
fi

# Support a flattened upload where scheduler.cpp was placed in the project root.
if [[ ! -f cpp/scheduler.cpp && -f scheduler.cpp ]]; then
  mkdir -p cpp
  cp scheduler.cpp cpp/scheduler.cpp
  echo "Copied scheduler.cpp into cpp/ for the pipeline."
fi

if [[ ! -f cpp/scheduler.cpp ]]; then
  echo "ERROR: Missing cpp/scheduler.cpp. Upload the scheduler source file to cpp/."
  exit 1
fi

mkdir -p data
if [[ ! -f data/workload.csv && -f data/workload.csv.example ]]; then
  cp data/workload.csv.example data/workload.csv
  echo "Created data/workload.csv from the example workload."
fi

if [[ ! -f data/workload.csv ]]; then
  echo "ERROR: Missing data/workload.csv. Add the process workload CSV under data/."
  exit 1
fi

if [[ ! -d .venv ]]; then
  python3 -m venv .venv
fi
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt

python run_pipeline.py

echo "Starting the dashboard at forwarded port 5000. Leave this terminal running."
exec python -m flask --app app run --host 0.0.0.0 --port 5000
