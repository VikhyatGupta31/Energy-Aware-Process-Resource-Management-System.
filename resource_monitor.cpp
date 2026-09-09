// resource_monitor.cpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <cctype>
#include <thread>
#include <chrono>
#include <ctime>
#include <algorithm>

using namespace std;

// 1. CPU USAGE 
// Reads the first line of /proc/stat, which looks like:
// cpu  user nice system idle iowait irq softirq steal guest guest_nice
// We take two snapshots a short time apart and compute the % of non-idle time.
struct CpuTimes {
    long user, nice, system, idle, iowait, irq, softirq, steal;
    long total() const { return user+nice+system+idle+iowait+irq+softirq+steal; }
    long idleAll() const { return idle + iowait; }
};

CpuTimes readCpuTimes() {
    ifstream file("/proc/stat");
    string cpu;
    CpuTimes t{};
    file >> cpu >> t.user >> t.nice >> t.system >> t.idle
         >> t.iowait >> t.irq >> t.softirq >> t.steal;
    return t;
}

double getCPUUsage() {
    CpuTimes t1 = readCpuTimes();
    this_thread::sleep_for(chrono::milliseconds(200)); // small sampling window
    CpuTimes t2 = readCpuTimes();

    long totalDelta = t2.total() - t1.total();
    long idleDelta  = t2.idleAll() - t1.idleAll();

    if (totalDelta <= 0) return 0.0;
    double usagePercent = 100.0 * (double)(totalDelta - idleDelta) / (double)totalDelta;
    return usagePercent;
}

// 2. MEMORY USAGE 
// Reads /proc/meminfo for MemTotal and MemAvailable, returns % used.
double getMemoryUsage() {
    ifstream file("/proc/meminfo");
    string key;
    long value;
    string unit;
    long memTotal = 0, memAvailable = 0;

    while (file >> key >> value >> unit) {
        if (key == "MemTotal:") memTotal = value;
        else if (key == "MemAvailable:") memAvailable = value;
        if (memTotal && memAvailable) break;
    }
    if (memTotal == 0) return 0.0;
    double usedPercent = 100.0 * (double)(memTotal - memAvailable) / (double)memTotal;
    return usedPercent;
}

// 3. DISK I/O 
// Reads /proc/diskstats and sums sectors read + written across all real disks
// (skips loop devices). Returns a simple I/O activity number (sectors).
long getDiskIO() {
    ifstream file("/proc/diskstats");
    string line;
    long totalSectors = 0;

    while (getline(file, line)) {
        istringstream iss(line);
        vector<string> fields;
        string token;
        while (iss >> token) fields.push_back(token);
        if (fields.size() < 10) continue;

        string devName = fields[2];
        if (devName.rfind("loop", 0) == 0) continue; // skip loop devices

        long sectorsRead    = stol(fields[5]);
        long sectorsWritten = stol(fields[9]);
        totalSectors += sectorsRead + sectorsWritten;
    }
    return totalSectors;
}

// 4. PROCESS LIST
// Scans /proc/[pid]/ directories, reads /proc/[pid]/comm for the process name.
// Returns a list of "pid,name" pairs.
struct ProcessInfo {
    int pid;
    string name;
};

vector<ProcessInfo> getProcessList() {
    vector<ProcessInfo> processes;
    DIR* procDir = opendir("/proc");
    if (!procDir) return processes;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        string dname = entry->d_name;
        bool isNumeric = !dname.empty() &&
            all_of(dname.begin(), dname.end(), [](char c){ return isdigit(c); });
        if (!isNumeric) continue;

        int pid = stoi(dname);
        string commPath = "/proc/" + dname + "/comm";
        ifstream commFile(commPath);
        string name;
        if (commFile) {
            getline(commFile, name);
            processes.push_back({pid, name});
        }
    }
    closedir(procDir);
    return processes;
}

// 5. RESOURCE MONITOR CLASS 
// Wraps everything above. Call sampleOnce() on a timer to append one row to the CSV.
// CSV columns match what Ruchi's `resource_logs` table expects:
// timestamp, cpu_usage, mem_usage, disk_io, process_count
class ResourceMonitor {
private:
    string csvPath;

public:
    ResourceMonitor(const string& path) : csvPath(path) {
        // Write header only if file doesn't exist yet
        ifstream check(csvPath);
        if (!check.good()) {
            ofstream out(csvPath);
            out << "timestamp,cpu_usage,mem_usage,disk_io,process_count\n";
        }
    }

    void sampleOnce() {
        double cpu = getCPUUsage();
        double mem = getMemoryUsage();
        long disk = getDiskIO();
        auto processes = getProcessList();

        time_t now = time(nullptr);
        ofstream out(csvPath, ios::app);
        out << now << "," << cpu << "," << mem << "," << disk << "," << processes.size() << "\n";

        cout << "[LOG] cpu=" << cpu << "% mem=" << mem << "% disk_io=" << disk
             << " processes=" << processes.size() << endl;
    }

    // Run continuous sampling every `intervalSeconds`, for `totalSamples` samples.
    void run(int intervalSeconds, int totalSamples) {
        for (int i = 0; i < totalSamples; i++) {
            sampleOnce();
            this_thread::sleep_for(chrono::seconds(intervalSeconds));
        }
    }
};

// MAIN (test driver) 
int main() {
    cout << "=== Vikhyat: Resource Monitor Test ===\n";
    ResourceMonitor monitor("resource_log.csv");
    monitor.run(1, 5); // sample every 1 second, 5 times
    cout << "Done. Check resource_log.csv for output.\n";
    return 0;
}
