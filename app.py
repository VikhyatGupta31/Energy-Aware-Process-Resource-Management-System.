from flask import Flask, jsonify, render_template
from db_loader import BACKEND, connect, initialize
app=Flask(__name__)
initialize()

def rows(sql):
    with connect() as db:
        cur=db.cursor();cur.execute(sql); cols=[c[0] for c in cur.description];return [dict(zip(cols,row)) for row in cur.fetchall()]

@app.get('/')
def index(): return render_template('index.html')

@app.get('/api/summary')
def summary():
    algorithms=rows('SELECT algorithm, AVG(waiting_time) AS waiting_time, AVG(turnaround_time) AS turnaround_time, SUM(estimated_energy) AS estimated_energy, COUNT(*) AS processes FROM scheduling_results GROUP BY algorithm ORDER BY algorithm')
    resources=rows('SELECT cpu_percent,memory_percent,disk_read_bytes,disk_write_bytes,active_processes,captured_at FROM resource_logs ORDER BY id DESC LIMIT 1')
    benchmark=rows('SELECT mode,elapsed_ms,rows_examined,explain_text FROM query_logs ORDER BY id DESC LIMIT 2')
    for item in algorithms:
        for key in ('waiting_time','turnaround_time','estimated_energy'):
            item[key]=float(item[key] or 0)
        item['processes']=int(item['processes'])
    for item in benchmark:
        item['elapsed_ms']=float(item['elapsed_ms'] or 0)
        item['rows_examined']=int(item['rows_examined'] or 0)
    if resources:
        for key in ('cpu_percent','memory_percent'):
            resources[0][key]=float(resources[0][key] or 0)
        for key in ('disk_read_bytes','disk_write_bytes','active_processes'):
            resources[0][key]=int(resources[0][key] or 0)
    return jsonify({'algorithms':algorithms,'resource':resources[0] if resources else None,'benchmark':benchmark,'backend':BACKEND})

if __name__=='__main__': app.run(host='127.0.0.1',port=5000,debug=False)
