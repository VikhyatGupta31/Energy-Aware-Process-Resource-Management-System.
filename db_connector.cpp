// db_connector.cpp

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <iostream>
#include <memory>

using namespace std;
using namespace sql;

// Central connection helper — call once, reuse the connection across inserts.
unique_ptr<Connection> connectToDB() {
    mysql::MySQL_Driver* driver = mysql::get_mysql_driver_instance();
    unique_ptr<Connection> conn(driver->connect("tcp://127.0.0.1:3306", "root", "your_password"));
    conn->setSchema("energy_aware_system");
    return conn;
}

// Insert one row into resource_logs. Called by Vikhyat's ResourceMonitor.
void insertResourceLog(Connection* conn, long timestamp, double cpu, double mem, long diskIO, int processCount) {
    unique_ptr<PreparedStatement> stmt(conn->prepareStatement(
        "INSERT INTO resource_logs (timestamp, cpu_usage, mem_usage, disk_io, process_count) "
        "VALUES (?, ?, ?, ?, ?)"));
    stmt->setInt64(1, timestamp);
    stmt->setDouble(2, cpu);
    stmt->setDouble(3, mem);
    stmt->setInt64(4, diskIO);
    stmt->setInt(5, processCount);
    stmt->execute();
}

// Insert one row into scheduling_results. Called after each scheduler run
// (FCFS, SJF, Round Robin, Priority, or Energy-Aware).
void insertSchedulingResult(Connection* conn, const string& algorithm, int pid,
                             int waitingTime, int turnaroundTime, int completionTime) {
    unique_ptr<PreparedStatement> stmt(conn->prepareStatement(
        "INSERT INTO scheduling_results (algorithm, pid, waiting_time, turnaround_time, completion_time) "
        "VALUES (?, ?, ?, ?, ?)"));
    stmt->setString(1, algorithm);
    stmt->setInt(2, pid);
    stmt->setInt(3, waitingTime);
    stmt->setInt(4, turnaroundTime);
    stmt->setInt(5, completionTime);
    stmt->execute();
}

// Insert one row into queries_log. Called during Ruchi's indexing experiments.
void insertQueryLog(Connection* conn, const string& queryText, double executionTimeMs, bool usedIndex) {
    unique_ptr<PreparedStatement> stmt(conn->prepareStatement(
        "INSERT INTO queries_log (query_text, execution_time_ms, used_index) VALUES (?, ?, ?)"));
    stmt->setString(1, queryText);
    stmt->setDouble(2, executionTimeMs);
    stmt->setBoolean(3, usedIndex);
    stmt->execute();
}

// MAIN (test driver) 

int main() {
    try {
        auto conn = connectToDB();
        cout << "Connected to MySQL successfully.\n";

        insertResourceLog(conn.get(), 1788943650, 12.5, 34.2, 174578, 50);
        insertSchedulingResult(conn.get(), "FCFS", 1, 0, 5, 5);
        insertQueryLog(conn.get(), "SELECT * FROM processes WHERE burst_time > 4", 2.31, false);

        cout << "Test inserts complete.\n";
    } catch (SQLException& e) {
        cerr << "MySQL error: " << e.what() << endl;
    }
    return 0;
}
