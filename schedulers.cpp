// schedulers.cpp


#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <climits>

using namespace std;

// Shared struct — every algorithm reads/writes these same fields.
// This is the format Ruchi's `scheduling_results` table should match.
struct Process {
    int pid;
    int arrivalTime;
    int burstTime;
    int priority;      // lower number = higher priority
    int remainingTime; // used by Round Robin
    int waitingTime = 0;
    int turnaroundTime = 0;
    int completionTime = 0;
};

void printResults(const string& algoName, vector<Process> processes) {
    cout << "\n=== " << algoName << " ===\n";
    cout << "PID\tArrival\tBurst\tWaiting\tTurnaround\tCompletion\n";
    double totalWait = 0, totalTurnaround = 0;
    for (auto& p : processes) {
        cout << p.pid << "\t" << p.arrivalTime << "\t" << p.burstTime << "\t"
             << p.waitingTime << "\t" << p.turnaroundTime << "\t\t" << p.completionTime << "\n";
        totalWait += p.waitingTime;
        totalTurnaround += p.turnaroundTime;
    }
    cout << "Average Waiting Time: " << totalWait / processes.size() << "\n";
    cout << "Average Turnaround Time: " << totalTurnaround / processes.size() << "\n";
}

// 1. FCFS
vector<Process> FCFS(vector<Process> processes) {
    sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrivalTime < b.arrivalTime;
    });

    int currentTime = 0;
    for (auto& p : processes) {
        if (currentTime < p.arrivalTime) currentTime = p.arrivalTime;
        p.waitingTime = currentTime - p.arrivalTime;
        currentTime += p.burstTime;
        p.completionTime = currentTime;
        p.turnaroundTime = p.completionTime - p.arrivalTime;
    }
    return processes;
}

// 2. SJF (non-preemptive)
vector<Process> SJF(vector<Process> processes) {
    int n = processes.size();
    vector<bool> done(n, false);
    int completed = 0, currentTime = 0;
    vector<Process> result;

    while (completed < n) {
        int idx = -1;
        int shortest = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!done[i] && processes[i].arrivalTime <= currentTime && processes[i].burstTime < shortest) {
                shortest = processes[i].burstTime;
                idx = i;
            }
        }
        if (idx == -1) { currentTime++; continue; } // no process has arrived yet

        Process p = processes[idx];
        p.waitingTime = currentTime - p.arrivalTime;
        currentTime += p.burstTime;
        p.completionTime = currentTime;
        p.turnaroundTime = p.completionTime - p.arrivalTime;

        result.push_back(p);
        done[idx] = true;
        completed++;
    }
    return result;
}

// 3. Round Robin
vector<Process> RoundRobin(vector<Process> processes, int quantum) {
    for (auto& p : processes) p.remainingTime = p.burstTime;

    sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrivalTime < b.arrivalTime;
    });

    queue<int> readyQueue;
    vector<bool> inQueue(processes.size(), false);
    int currentTime = 0, completed = 0;
    int n = processes.size();

    readyQueue.push(0);
    inQueue[0] = true;

    while (completed < n) {
        if (readyQueue.empty()) { currentTime++; continue; }

        int idx = readyQueue.front();
        readyQueue.pop();
        Process& p = processes[idx];

        int execTime = min(quantum, p.remainingTime);
        currentTime += execTime;
        p.remainingTime -= execTime;

        // Add any newly arrived processes to the queue
        for (int i = 0; i < n; i++) {
            if (!inQueue[i] && processes[i].arrivalTime <= currentTime && processes[i].remainingTime > 0) {
                readyQueue.push(i);
                inQueue[i] = true;
            }
        }

        if (p.remainingTime > 0) {
            readyQueue.push(idx); // re-queue if not finished
        } else {
            p.completionTime = currentTime;
            p.turnaroundTime = p.completionTime - p.arrivalTime;
            p.waitingTime = p.turnaroundTime - p.burstTime;
            completed++;
        }
    }
    return processes;
}

// MAIN (test driver) 
int main() {
    // Sample process set — Ansh should use this SAME set for Priority Scheduling
    // so Phase 3 comparisons are apples-to-apples.
    vector<Process> processes = {
        {1, 0, 5, 2, 0}, // pid, arrival, burst, priority, remaining
        {2, 1, 3, 1, 0},
        {3, 2, 8, 3, 0},
        {4, 3, 6, 2, 0},
        {5, 4, 2, 1, 0}
    };

    printResults("FCFS", FCFS(processes));
    printResults("SJF", SJF(processes));
    printResults("Round Robin (quantum=2)", RoundRobin(processes, 2));

    return 0;
}
