// workload.h
// Owner: agreed by all three (this is the "common shared data" sync point
// we talked about before Phase 1 started).
// Returns the SAME process set every time, so FCFS/SJF/RR/Priority/EnergyAware
// are all tested on identical input — required for a fair Phase 3 comparison.

#ifndef WORKLOAD_H
#define WORKLOAD_H

#include <vector>
#include "process.h"

inline std::vector<Process> getSharedWorkload() {
    return {
        // pid, arrival, burst, priority, cpuIntensity, memIntensity, diskIntensity
        {1, 0, 5, 2, 1.2, 0.8, 0.5},
        {2, 1, 3, 1, 0.6, 0.4, 0.2},
        {3, 2, 8, 3, 1.8, 1.5, 1.2},
        {4, 3, 6, 2, 1.0, 1.0, 1.0},
        {5, 4, 2, 1, 0.4, 0.3, 0.1}
    };
}

#endif
