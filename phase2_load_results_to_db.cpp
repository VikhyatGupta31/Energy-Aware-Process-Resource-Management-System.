// load_results_to_db.cpp
// Owner: Ruchi (Phase 2)
// Reads scheduling_results_ready.csv (produced by Vikhyat's integration_pipeline.cpp)
// and inserts every row into MySQL via insertSchedulingResult().
// This is the actual "integration" step: OS-side output -> DBMS-side storage.

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>

using namespace std;
using namespace sql;

unique_ptr<Connection> connectToDB() {
    mysql::MySQL_Driver* driver = mysql::get_mysql_driver_instance();
    unique_ptr<Connection> conn(driver->connect("tcp://127.0.0.1:3306", "root", "your_password"));
    conn->setSchema("energy_aware_system");
    return conn;
}

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

void insertEnergyEstimate(Connection* conn, long timestamp, double energy, const string& sourceType) {
    unique_ptr<PreparedStatement> stmt(conn->prepareStatement(
        "INSERT INTO energy_estimates (timestamp, estimated_energy, source_type) VALUES (?, ?, ?)"));
    stmt->setInt64(1, timestamp);
    stmt->setDouble(2, energy);
    stmt->setString(3, sourceType);
    stmt->execute();
}

int main() {
    ifstream csv("scheduling_results_ready.csv");
    if (!csv.is_open()) {
        cerr << "Could not open scheduling_results_ready.csv — run Vikhyat's "
             << "integration_pipeline.cpp first.\n";
        return 1;
    }

    try {
        auto conn = connectToDB();
        cout << "Connected to MySQL. Loading results...\n";

        string line;
        getline(csv, line); // skip header
        int rowCount = 0;

        while (getline(csv, line)) {
            istringstream iss(line);
            string algorithm, pidStr, waitStr, turnaroundStr, completionStr, energyStr;
            getline(iss, algorithm, ',');
            getline(iss, pidStr, ',');
            getline(iss, waitStr, ',');
            getline(iss, turnaroundStr, ',');
            getline(iss, completionStr, ',');
            getline(iss, energyStr, ',');

            int pid = stoi(pidStr);
            int waiting = stoi(waitStr);
            int turnaround = stoi(turnaroundStr);
            int completion = stoi(completionStr);
            double energy = stod(energyStr);

            insertSchedulingResult(conn.get(), algorithm, pid, waiting, turnaround, completion);
            if (energy > 0) {
                insertEnergyEstimate(conn.get(), time(nullptr), energy, "scheduling:" + algorithm);
            }
            rowCount++;
        }

        cout << "Inserted " << rowCount << " scheduling results into MySQL.\n";
    } catch (SQLException& e) {
        cerr << "MySQL error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
