// process.h
// Shared by Vikhyat, Ruchi, and Ansh's code.
// This is the ONE definition of a "process" every module must use —
// Phase 1 established the fields; Phase 2 adds resource-intensity fields
// so the energy-aware scheduler has something to work with.

#ifndef PROCESS_H
#define PROCESS_H

#include <string>

struct Process {
    int pid;
    int arrivalTime;
    int burstTime;
    int priority;        // lower number = higher priority

    // NEW in Phase 2: per-process resource intensity, used by the
    // energy-aware scheduler and the energy estimator. In a real system
    // these would come from profiling; here they're set per test workload.
    double cpuIntensity = 1.0;  // relative multiplier, 1.0 = average
    double memIntensity = 1.0;
    double diskIntensity = 1.0;

    // filled in by whichever scheduler runs this process
    int remainingTime = 0;
    int waitingTime = 0;
    int turnaroundTime = 0;
    int completionTime = 0;
    double estimatedEnergy = 0.0; // filled in by the energy-aware scheduler
};

// Energy coefficients — must match Ansh's energy_estimator.py exactly,
// or the two will disagree when results are compared on the dashboard.
constexpr double C_CPU = 0.90;
constexpr double C_DISK = 0.02;
constexpr double C_MEM = 0.30;

#endif
