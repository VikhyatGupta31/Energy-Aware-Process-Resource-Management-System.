CREATE DATABASE IF NOT EXISTS energy_aware_os;
USE energy_aware_os;
CREATE TABLE IF NOT EXISTS processes (
  pid INT PRIMARY KEY, arrival_time INT NOT NULL, burst_time INT NOT NULL, priority INT NOT NULL
);
CREATE TABLE IF NOT EXISTS scheduling_results (
  id BIGINT AUTO_INCREMENT PRIMARY KEY, run_id VARCHAR(64) NOT NULL, algorithm VARCHAR(32) NOT NULL,
  pid INT NOT NULL, arrival_time INT NOT NULL, burst_time INT NOT NULL, priority INT NOT NULL,
  waiting_time DOUBLE NOT NULL, turnaround_time DOUBLE NOT NULL,
  completion_time DOUBLE NOT NULL, estimated_energy DOUBLE NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_results_run_algorithm (run_id, algorithm)
);
CREATE TABLE IF NOT EXISTS resource_logs (
  id BIGINT AUTO_INCREMENT PRIMARY KEY, cpu_percent DOUBLE NOT NULL, memory_percent DOUBLE NOT NULL,
  disk_read_bytes BIGINT NOT NULL, disk_write_bytes BIGINT NOT NULL, active_processes INT NOT NULL,
  captured_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS query_logs (
  id BIGINT AUTO_INCREMENT PRIMARY KEY, run_id VARCHAR(64) NOT NULL, mode VARCHAR(16) NOT NULL,
  elapsed_ms DOUBLE NOT NULL, rows_examined INT NOT NULL, explain_text TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
