import csv, os, sqlite3, time
from pathlib import Path
ROOT=Path(__file__).resolve().parent; DATA=ROOT/'data'; DB_PATH=DATA/'project.db'; BACKEND=os.getenv('DB_BACKEND','sqlite').lower()

def connect():
    if BACKEND=='mysql':
        import mysql.connector
        return mysql.connector.connect(host=os.getenv('MYSQL_HOST','127.0.0.1'),port=int(os.getenv('MYSQL_PORT','3306')),user=os.getenv('MYSQL_USER','root'),password=os.getenv('MYSQL_PASSWORD',''),database=os.getenv('MYSQL_DATABASE','energy_aware_os'))
    return sqlite3.connect(DB_PATH)

def initialize():
    if BACKEND=='mysql':
        statements=[
        "CREATE TABLE IF NOT EXISTS processes(pid INT PRIMARY KEY,arrival_time INT NOT NULL,burst_time INT NOT NULL,priority INT NOT NULL)",
        "CREATE TABLE IF NOT EXISTS scheduling_results(id BIGINT AUTO_INCREMENT PRIMARY KEY,run_id VARCHAR(64) NOT NULL,algorithm VARCHAR(32) NOT NULL,pid INT NOT NULL,arrival_time INT NOT NULL,burst_time INT NOT NULL,priority INT NOT NULL,waiting_time DOUBLE NOT NULL,turnaround_time DOUBLE NOT NULL,completion_time DOUBLE NOT NULL,estimated_energy DOUBLE NOT NULL,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,INDEX idx_results_run_algorithm(run_id,algorithm))",
        "CREATE TABLE IF NOT EXISTS resource_logs(id BIGINT AUTO_INCREMENT PRIMARY KEY,cpu_percent DOUBLE NOT NULL,memory_percent DOUBLE NOT NULL,disk_read_bytes BIGINT NOT NULL,disk_write_bytes BIGINT NOT NULL,active_processes INT NOT NULL,captured_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS query_logs(id BIGINT AUTO_INCREMENT PRIMARY KEY,run_id VARCHAR(64) NOT NULL,mode VARCHAR(32) NOT NULL,elapsed_ms DOUBLE NOT NULL,rows_examined INT NOT NULL,explain_text TEXT,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS benchmark_data(id INT PRIMARY KEY,category VARCHAR(32) NOT NULL,value INT NOT NULL)"]
        with connect() as db:
            cur=db.cursor()
            for sql in statements: cur.execute(sql)
            db.commit()
    else:
        with sqlite3.connect(DB_PATH) as db:
            db.executescript('''CREATE TABLE IF NOT EXISTS processes(pid INTEGER PRIMARY KEY,arrival_time INTEGER NOT NULL,burst_time INTEGER NOT NULL,priority INTEGER NOT NULL); CREATE TABLE IF NOT EXISTS scheduling_results(id INTEGER PRIMARY KEY,run_id TEXT NOT NULL,algorithm TEXT NOT NULL,pid INTEGER NOT NULL,arrival_time INTEGER NOT NULL,burst_time INTEGER NOT NULL,priority INTEGER NOT NULL,waiting_time REAL NOT NULL,turnaround_time REAL NOT NULL,completion_time REAL NOT NULL,estimated_energy REAL NOT NULL,created_at TEXT DEFAULT CURRENT_TIMESTAMP); CREATE INDEX IF NOT EXISTS idx_results_run_algorithm ON scheduling_results(run_id,algorithm); CREATE TABLE IF NOT EXISTS resource_logs(id INTEGER PRIMARY KEY,cpu_percent REAL NOT NULL,memory_percent REAL NOT NULL,disk_read_bytes INTEGER NOT NULL,disk_write_bytes INTEGER NOT NULL,active_processes INTEGER NOT NULL,captured_at TEXT DEFAULT CURRENT_TIMESTAMP); CREATE TABLE IF NOT EXISTS query_logs(id INTEGER PRIMARY KEY,run_id TEXT NOT NULL,mode TEXT NOT NULL,elapsed_ms REAL NOT NULL,rows_examined INTEGER NOT NULL,explain_text TEXT,created_at TEXT DEFAULT CURRENT_TIMESTAMP); CREATE TABLE IF NOT EXISTS benchmark_data(id INTEGER PRIMARY KEY,category TEXT NOT NULL,value INTEGER NOT NULL);''')

def ingest_csvs():
    initialize(); results=[]
    with (DATA/'scheduling_results.csv').open(newline='',encoding='utf-8-sig') as f:
        for r in csv.DictReader(f): results.append(tuple(r[k] for k in ('run_id','algorithm','pid','arrival_time','burst_time','priority','waiting_time','turnaround_time','completion_time','estimated_energy')))
    with (DATA/'resource_logs.csv').open(newline='',encoding='utf-8-sig') as f: res=next(csv.DictReader(f))
    with (DATA/'workload.csv').open(newline='',encoding='utf-8-sig') as f:
        processes=[tuple(r[k] for k in ('pid','arrival_time','burst_time','priority')) for r in csv.DictReader(f)]
    ph='%s' if BACKEND=='mysql' else '?'
    with connect() as db:
        cur=db.cursor(); cur.execute('DELETE FROM scheduling_results'); cur.execute('DELETE FROM processes')
        cur.executemany(f'INSERT INTO processes(pid,arrival_time,burst_time,priority) VALUES ({",".join([ph]*4)})',processes)
        cur.executemany(f'INSERT INTO scheduling_results(run_id,algorithm,pid,arrival_time,burst_time,priority,waiting_time,turnaround_time,completion_time,estimated_energy) VALUES ({",".join([ph]*10)})',results)
        cur.execute(f'INSERT INTO resource_logs(cpu_percent,memory_percent,disk_read_bytes,disk_write_bytes,active_processes) VALUES ({",".join([ph]*5)})',(res['cpu_percent'],res['memory_percent'],res['disk_read_bytes'],res['disk_write_bytes'],res['active_processes']))
        db.commit()
    print(f'Loaded {len(results)} scheduling rows into {BACKEND}.')

def run_benchmark():
    initialize(); ph='%s' if BACKEND=='mysql' else '?'; run_id=str(int(time.time()))
    with connect() as db:
        cur=db.cursor(); cur.execute('SELECT COUNT(*) FROM benchmark_data'); count=cur.fetchone()[0]
        if count<20000:
            if BACKEND=='mysql': cur.executemany('INSERT IGNORE INTO benchmark_data(id,category,value) VALUES (%s,%s,%s)',[(i,f'group-{i%100}',i%1000) for i in range(1,20001)])
            else: cur.executemany('INSERT OR IGNORE INTO benchmark_data(id,category,value) VALUES (?,?,?)',[(i,f'group-{i%100}',i%1000) for i in range(1,20001)])
        if BACKEND=='mysql':
            cur.execute('SHOW INDEX FROM benchmark_data WHERE Key_name=%s',('idx_benchmark_category',))
            if cur.fetchone(): cur.execute('DROP INDEX idx_benchmark_category ON benchmark_data')
        else: cur.execute('DROP INDEX IF EXISTS idx_benchmark_category')
        db.commit(); output=[]
        for mode in ('without_index','with_index'):
            if mode=='with_index': cur.execute('CREATE INDEX idx_benchmark_category ON benchmark_data(category)')
            total=0; start=time.perf_counter()
            for _ in range(80):
                cur.execute(f'SELECT id,value FROM benchmark_data WHERE category={ph} ORDER BY value',('group-42',)); total+=len(cur.fetchall())
            elapsed=(time.perf_counter()-start)*1000
            cur.execute(f'EXPLAIN SELECT id,value FROM benchmark_data WHERE category={ph} ORDER BY value',('group-42',)); plan=' | '.join(str(x) for x in cur.fetchone())
            output.append((mode,elapsed,total,plan))
        cur.executemany(f'INSERT INTO query_logs(run_id,mode,elapsed_ms,rows_examined,explain_text) VALUES ({",".join([ph]*5)})',[(run_id,*x) for x in output]); db.commit()
    with (DATA/'query_benchmark.csv').open('w',newline='',encoding='utf-8') as f:
        w=csv.writer(f);w.writerow(['mode','elapsed_ms','rows_returned','explain']);w.writerows(output)
    print('Query benchmark: '+', '.join(f'{m} {ms:.2f} ms' for m,ms,_,_ in output))
