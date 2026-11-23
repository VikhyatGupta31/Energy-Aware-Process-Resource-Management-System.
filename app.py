import csv
import time
from datetime import datetime, timezone
from pathlib import Path

import psutil
from flask import Flask, jsonify, render_template, request
from db_loader import BACKEND, connect, initialize
from simulator import simulate
app=Flask(__name__)
initialize()
ROOT = Path(__file__).resolve().parent
_last_disk = None

def rows(sql):
    with connect() as db:
        cur=db.cursor();cur.execute(sql); cols=[c[0] for c in cur.description];return [dict(zip(cols,row)) for row in cur.fetchall()]

@app.get('/')
def index(): return render_template('index.html')

@app.get('/api/live')
def live_metrics():
    """Return a fresh host-level resource sample; it is not a simulation result."""
    global _last_disk
    cpu = psutil.cpu_percent(interval=0.15)
    memory = psutil.virtual_memory()
    now = time.monotonic()
    counters = psutil.disk_io_counters()
    read_rate = write_rate = 0.0
    if counters:
        current = (now, counters.read_bytes, counters.write_bytes)
        if _last_disk:
            elapsed = max(current[0] - _last_disk[0], 0.001)
            read_rate = max(0, current[1] - _last_disk[1]) / elapsed
            write_rate = max(0, current[2] - _last_disk[2]) / elapsed
        _last_disk = current

    top = []
    for proc in psutil.process_iter(['pid', 'name', 'memory_percent', 'cpu_percent']):
        try:
            info = proc.info
            top.append({'pid': info['pid'], 'name': info.get('name') or 'Unknown',
                        'memory_percent': round(info.get('memory_percent') or 0, 2),
                        'cpu_percent': round(info.get('cpu_percent') or 0, 1)})
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            continue
    top.sort(key=lambda item: item['memory_percent'], reverse=True)
    return jsonify({
        'cpu_percent': round(cpu, 1), 'memory_percent': round(memory.percent, 1),
        'memory_used': memory.used, 'memory_total': memory.total,
        'active_processes': len(psutil.pids()), 'disk_read_bytes_per_sec': round(read_rate),
        'disk_write_bytes_per_sec': round(write_rate), 'top_processes': top[:8],
        'sampled_at': datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds'),
        'source': 'live operating-system sample',
    })

@app.post('/api/simulate')
def run_simulation():
    payload = request.get_json(silent=True) or {}
    workload_name = payload.get('workload_name', 'baseline')
    algorithm = payload.get('algorithm', 'FCFS')
    if workload_name not in {'baseline', 'cpu_heavy', 'short_jobs', 'staggered_arrivals'}:
        return jsonify({'error': 'Unknown workload.'}), 400
    workload_path = ROOT / 'data' / 'workloads' / f'{workload_name}.csv'
    if not workload_path.is_file():
        return jsonify({'error': 'Run the project pipeline first to generate workloads.'}), 409
    try:
        with workload_path.open(newline='', encoding='utf-8-sig') as file:
            process_rows = list(csv.DictReader(file))
        result = simulate(process_rows, algorithm)
        result['workload_name'] = workload_name
        result['simulation_only'] = True
        return jsonify(result)
    except ValueError as exc:
        return jsonify({'error': str(exc)}), 400

@app.get('/api/summary')
def summary():
    latest=rows('SELECT run_id FROM scheduling_results ORDER BY id DESC LIMIT 1')
    run_id=latest[0]['run_id'] if latest else None
    algorithms=[]; resources=[]; workloads=[]
    if run_id:
        with connect() as db:
            cur=db.cursor()
            query='SELECT workload_name,algorithm,trial_number,AVG(waiting_time) AS waiting_time,AVG(turnaround_time) AS turnaround_time,AVG(response_time) AS response_time,SUM(estimated_energy) AS estimated_energy,COUNT(*) AS processes,MIN(arrival_time) AS first_arrival,MAX(completion_time) AS last_completion FROM scheduling_results WHERE run_id=? GROUP BY workload_name,algorithm,trial_number ORDER BY workload_name,algorithm,trial_number'
            if BACKEND=='mysql': query=query.replace('run_id=?','run_id=%s')
            cur.execute(query,(run_id,))
            cols=[c[0] for c in cur.description]
            trial_rows=[dict(zip(cols,row)) for row in cur.fetchall()]
            grouped={}
            for item in trial_rows:
                grouped.setdefault((item['workload_name'],item['algorithm']),[]).append(item)
            for (workload_name,algorithm), trials in grouped.items():
                def mean(field):
                    return sum(float(t[field] or 0) for t in trials)/len(trials)
                algorithms.append({
                    'workload_name':workload_name,'algorithm':algorithm,
                    'processes':round(mean('processes')),'waiting_time':mean('waiting_time'),
                    'turnaround_time':mean('turnaround_time'),'response_time':mean('response_time'),
                    'estimated_energy':mean('estimated_energy'),
                    'throughput':sum(float(t['processes'])/max(1,float(t['last_completion'])-float(t['first_arrival'])) for t in trials)/len(trials),
                    'trials':len(trials),'waiting_min':min(float(t['waiting_time']) for t in trials),
                    'waiting_max':max(float(t['waiting_time']) for t in trials),
                })
            cur.execute('SELECT workload_name,trial_number,cpu_percent,memory_percent,disk_read_bytes,disk_write_bytes,active_processes,captured_at FROM resource_logs WHERE run_id=? ORDER BY id' if BACKEND!='mysql' else 'SELECT workload_name,trial_number,cpu_percent,memory_percent,disk_read_bytes,disk_write_bytes,active_processes,captured_at FROM resource_logs WHERE run_id=%s ORDER BY id',(run_id,))
            cols=[c[0] for c in cur.description]; resources=[dict(zip(cols,row)) for row in cur.fetchall()]
        workloads=sorted({item['workload_name'] for item in algorithms})
    benchmark=rows('SELECT mode,elapsed_ms,rows_examined,explain_text FROM query_logs ORDER BY id DESC LIMIT 2')
    for item in algorithms:
        for key in ('waiting_time','turnaround_time','response_time','estimated_energy','throughput','waiting_min','waiting_max'):
            item[key]=float(item[key] or 0)
        item['processes']=int(item['processes'])
        item['trials']=int(item['trials'])
    for item in benchmark:
        item['elapsed_ms']=float(item['elapsed_ms'] or 0)
        item['rows_examined']=int(item['rows_examined'] or 0)
    for resource in resources:
        for key in ('cpu_percent','memory_percent'): resource[key]=float(resource[key] or 0)
        for key in ('disk_read_bytes','disk_write_bytes','active_processes','trial_number'): resource[key]=int(resource[key] or 0)
    return jsonify({'algorithms':algorithms,'workloads':workloads,'resources':resources,'run_id':run_id,'benchmark':benchmark,'backend':BACKEND})

if __name__=='__main__': app.run(host='127.0.0.1',port=5000,debug=False)
