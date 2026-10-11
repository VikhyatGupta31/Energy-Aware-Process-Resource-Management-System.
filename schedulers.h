// schedulers.h
// Owner: Vikhyat
// Same FCFS/SJF/RoundRobin logic from Phase 1, now as a header so Ansh's
// energy-aware scheduler and the integration pipeline can both include it.

#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <vector>
#include <algorithm>
#include <queue>
#include <climits>
#include "../common/process.h"

inline std::vector<Process> FCFS(std::vector<Process> processes) {
    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
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

inline std::vector<Process> SJF(std::vector<Process> processes) {
    int n = processes.size();
    std::vector<bool> done(n, false);
    int completed = 0, currentTime = 0;
    std::vector<Process> result;

    while (completed < n) {
        int idx = -1, shortest = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!done[i] && processes[i].arrivalTime <= currentTime && processes[i].burstTime < shortest) {
                shortest = processes[i].burstTime;
                idx = i;
            }
        }
        if (idx == -1) { currentTime++; continue; }

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

inline std::vector<Process> RoundRobin(std::vector<Process> processes, int quantum) {
    for (auto& p : processes) p.remainingTime = p.burstTime;
    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrivalTime < b.arrivalTime;
    });

    std::queue<int> readyQueue;
    std::vector<bool> inQueue(processes.size(), false);
    int currentTime = 0, completed = 0;
    int n = processes.size();

    readyQueue.push(0);
    inQueue[0] = true;

    while (completed < n) {
        if (readyQueue.empty()) { currentTime++; continue; }
        int idx = readyQueue.front();
        readyQueue.pop();
        Process& p = processes[idx];

        int execTime = std::min(quantum, p.remainingTime);
        currentTime += execTime;
        p.remainingTime -= execTime;

        for (int i = 0; i < n; i++) {
            if (!inQueue[i] && processes[i].arrivalTime <= currentTime && processes[i].remainingTime > 0) {
                readyQueue.push(i);
                inQueue[i] = true;
            }
        }
        if (p.remainingTime > 0) {
            readyQueue.push(idx);
        } else {
            p.completionTime = currentTime;
            p.turnaroundTime = p.completionTime - p.arrivalTime;
            p.waitingTime = p.turnaroundTime - p.burstTime;
            completed++;
        }
    }
    return processes;
}

#endif
