-- dashboard_queries.sql
-- Owner: Ruchi (written for Ansh's dashboard to call)
-- These are the exact aggregation queries the Phase 2 dashboard uses.
-- Tested logic — run these yourself against your MySQL instance to confirm
-- output shape before wiring them into app.py.

-- 1. Average waiting/turnaround time per scheduling algorithm (for bar chart)
SELECT algorithm,
       AVG(waiting_time) AS avg_waiting_time,
       AVG(turnaround_time) AS avg_turnaround_time
FROM scheduling_results
GROUP BY algorithm;

-- 2. Total estimated energy per algorithm (for the energy comparison chart)
SELECT source_type, SUM(estimated_energy) AS total_energy
FROM energy_estimates
WHERE source_type LIKE 'scheduling:%'
GROUP BY source_type;

-- 3. Resource usage over time (for the CPU/memory line chart)
SELECT timestamp, cpu_usage, mem_usage, disk_io
FROM resource_logs
ORDER BY timestamp ASC
LIMIT 50;

-- 4. Indexed vs non-indexed query performance (for the query benchmark chart)
SELECT used_index, AVG(execution_time_ms) AS avg_time_ms, COUNT(*) AS num_runs
FROM queries_log
GROUP BY used_index;
