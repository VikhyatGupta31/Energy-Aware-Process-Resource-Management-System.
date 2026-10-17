// integration_pipeline.cpp
// Owner: Vikhyat (Phase 2)
// Ties together: resource monitoring + all 5 schedulers (FCFS, SJF, RR,
// Priority, Energy-Aware) running on the SAME shared workload.
// Outputs a CSV formatted exactly like Ruchi's `scheduling_results` table,
// ready to be inserted (see ruchi/load_results_to_db.cpp for the DB step).

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "../common/process.h"
#include "../common/workload.h"
#include "schedulers.h"
#include "../ansh/phase2_priority_and_energy_scheduler.h"

using namespace std;

void writeResultsToCSV(ofstream& out, const string& algorithm, const vector<Process>& results) {
    for (const auto& p : results) {
        out << algorithm << "," << p.pid << "," << p.waitingTime << ","
            << p.turnaroundTime << "," << p.completionTime << "," << p.estimatedEnergy << "\n";
    }
}

void printSummary(const string& algorithm, const vector<Process>& results) {
    double totalWait = 0, totalTurnaround = 0;
    for (const auto& p : results) {
        totalWait += p.waitingTime;
        totalTurnaround += p.turnaroundTime;
    }
    cout << algorithm << ": avg waiting=" << (totalWait / results.size())
         << " avg turnaround=" << (totalTurnaround / results.size()) << "\n";
}

int main() {
    cout << "=== Vikhyat: Phase 2 Integration Pipeline ===\n";
    cout << "Running all 5 schedulers on the shared workload...\n\n";

    vector<Process> workload = getSharedWorkload();

    auto fcfsResult = FCFS(workload);
    auto sjfResult = SJF(workload);
    auto rrResult = RoundRobin(workload, 2);
    auto priorityResult = PriorityScheduling(workload);
    auto energyAwareResult = EnergyAwareScheduling(workload);

    printSummary("FCFS", fcfsResult);
    printSummary("SJF", sjfResult);
    printSummary("RoundRobin", rrResult);
    printSummary("Priority", priorityResult);
    printSummary("EnergyAware", energyAwareResult);

    // Write everything to a CSV matching scheduling_results table columns.
    // Ruchi's load_results_to_db.cpp reads this file and inserts each row.
    ofstream out("scheduling_results_ready.csv");
    out << "algorithm,pid,waiting_time,turnaround_time,completion_time,estimated_energy\n";
    writeResultsToCSV(out, "FCFS", fcfsResult);
    writeResultsToCSV(out, "SJF", sjfResult);
    writeResultsToCSV(out, "RoundRobin", rrResult);
    writeResultsToCSV(out, "Priority", priorityResult);
    writeResultsToCSV(out, "EnergyAware", energyAwareResult);
    out.close();

    cout << "\nAll results written to scheduling_results_ready.csv\n";
    cout << "Ruchi: run load_results_to_db.cpp to insert this into MySQL.\n";

    return 0;
}
