-- schema.sql

CREATE DATABASE IF NOT EXISTS energy_aware_system;
USE energy_aware_system;

-- 1. processes: the synthetic workload used for all scheduling tests
CREATE TABLE IF NOT EXISTS processes (
    pid INT PRIMARY KEY,
    name VARCHAR(100),
    arrival_time INT NOT NULL,
    burst_time INT NOT NULL,
    priority INT
);

-- 2. resource_logs: output of Vikhyat's ResourceMonitor, one row per sample
CREATE TABLE IF NOT EXISTS resource_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp BIGINT NOT NULL,
    cpu_usage DECIMAL(6,2),
    mem_usage DECIMAL(6,2),
    disk_io BIGINT,
    process_count INT
);

-- 3. scheduling_results: output of every scheduler run (FCFS/SJF/RR/Priority/Energy-Aware)
CREATE TABLE IF NOT EXISTS scheduling_results (
    id INT AUTO_INCREMENT PRIMARY KEY,
    algorithm VARCHAR(30) NOT NULL,
    pid INT NOT NULL,
    waiting_time INT,
    turnaround_time INT,
    completion_time INT,
    FOREIGN KEY (pid) REFERENCES processes(pid)
);

-- 4. queries_log: results of Ruchi's indexed vs non-indexed query experiments
CREATE TABLE IF NOT EXISTS queries_log (
    id INT AUTO_INCREMENT PRIMARY KEY,
    query_text TEXT,
    execution_time_ms DECIMAL(10,3),
    used_index BOOLEAN
);

-- 5. energy_estimates: output of Ansh's estimateEnergy() function
CREATE TABLE IF NOT EXISTS energy_estimates (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp BIGINT NOT NULL,
    estimated_energy DECIMAL(12,4),
    source_type VARCHAR(30)  -- e.g. 'scheduling', 'query', 'monitor_sample'
);

-- Sample data + EXPLAIN practice (for Phase 1 self-testing) 

INSERT INTO processes (pid, name, arrival_time, burst_time, priority) VALUES
(1, 'procA', 0, 5, 2),
(2, 'procB', 1, 3, 1),
(3, 'procC', 2, 8, 3),
(4, 'procD', 3, 6, 2),
(5, 'procE', 4, 2, 1);

-- Non-indexed query on burst_time
EXPLAIN SELECT * FROM processes WHERE burst_time > 4;

-- Add an index and compare the EXPLAIN output before/after
CREATE INDEX idx_burst_time ON processes(burst_time);
EXPLAIN SELECT * FROM processes WHERE burst_time > 4;
