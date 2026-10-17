import csv
import os
import sqlite3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DATA = ROOT / "data"
CPP = ROOT / "cpp" / "scheduler.cpp"
EXE = ROOT / ("scheduler.exe" if os.name == "nt" else "scheduler")

def compile_scheduler():
    if os.name == "nt" and subprocess.run(["where", "cl"], capture_output=True, text=True).returncode == 0:
        subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/O2", str(CPP), f"/Fe:{EXE}"], cwd=ROOT, check=True)
    else:
        compiler = "g++"
        subprocess.run([compiler, "-std=c++17", "-O2", str(CPP), "-o", str(EXE)], cwd=ROOT, check=True)

def main():
    DATA.mkdir(exist_ok=True)
    compile_scheduler()
    subprocess.run([str(EXE), "data/workload.csv"], cwd=ROOT, check=True)
    from db_loader import ingest_csvs, run_benchmark
    ingest_csvs()
    run_benchmark()
    print("Pipeline complete. Start the dashboard with: python app.py")

if __name__ == "__main__":
    try: main()
    except subprocess.CalledProcessError as exc:
        print(f"Pipeline failed (exit {exc.returncode}). Check that a C++17 compiler is installed and available on PATH.", file=sys.stderr)
        raise SystemExit(exc.returncode)
