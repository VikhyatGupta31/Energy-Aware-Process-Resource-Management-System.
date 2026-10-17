#include <algorithm>
#include <chrono>
#include <cctype>
#include <thread>
#include <tuple>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <dirent.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

struct Process { int pid, arrival, burst, priority; };
struct Row { int pid, arrival, burst, priority; double waiting, turnaround, completion; };
struct Resource { double cpu=0, memory=0; long long read=0, write=0; int processes=0; };

static std::vector<Process> workload(const std::string& path) {
    std::ifstream in(path); std::string line; std::vector<Process> ps;
    if (!in) throw std::runtime_error("Cannot open workload CSV: " + path);
    std::getline(in,line);
    while (std::getline(in,line)) {
        std::stringstream ss(line); std::string v; Process p{};
        if (!std::getline(ss,v,',')) continue; p.pid=std::stoi(v);
        if (!std::getline(ss,v,',')) continue; p.arrival=std::stoi(v);
        if (!std::getline(ss,v,',')) continue; p.burst=std::stoi(v);
        if (!std::getline(ss,v,',')) continue; p.priority=std::stoi(v);
        if(p.arrival<0 || p.burst<=0) throw std::runtime_error("Arrival must be >= 0 and burst > 0");
        ps.push_back(p);
    }
    if(ps.empty()) throw std::runtime_error("Workload has no processes"); return ps;
}
static std::vector<Row> finish(const std::vector<Process>& ps, const std::vector<int>& order) {
    std::vector<double> done(ps.size(),0); double t=0;
    for(int i:order) { t=std::max(t,(double)ps[i].arrival)+ps[i].burst; done[i]=t; }
    std::vector<Row> out; for(size_t i=0;i<ps.size();++i) out.push_back({ps[i].pid,ps[i].arrival,ps[i].burst,ps[i].priority,done[i]-ps[i].arrival-ps[i].burst,done[i]-ps[i].arrival,done[i]}); return out;
}
static std::vector<Row> nonpreemptive(const std::vector<Process>& ps, int kind) {
    std::vector<int> order; std::vector<bool> used(ps.size(),false); double t=0;
    while(order.size()<ps.size()) { int best=-1;
      for(size_t i=0;i<ps.size();++i) if(!used[i] && ps[i].arrival<=t) {
        if(best<0 || (kind==0 && std::tuple<int,size_t>(ps[i].arrival,i)<std::tuple<int,size_t>(ps[best].arrival,(size_t)best)) ||
          (kind==1 && std::tie(ps[i].burst,ps[i].arrival)<std::tie(ps[best].burst,ps[best].arrival)) ||
          (kind==2 && std::tie(ps[i].priority,ps[i].arrival)<std::tie(ps[best].priority,ps[best].arrival))) best=(int)i;
      }
      if(best<0) { int next=1000000000; for(size_t i=0;i<ps.size();++i) if(!used[i]) next=std::min(next,ps[i].arrival); t=next; continue; }
      order.push_back(best); used[best]=true; t+=ps[best].burst;
    }
    return finish(ps,order);
}
static std::vector<Row> roundRobin(const std::vector<Process>& ps, int quantum) {
    std::vector<int> rem; for(auto&p:ps) rem.push_back(p.burst);
    std::vector<double> done(ps.size()); std::vector<bool> queued(ps.size(),false); std::vector<int> q; double t=0; size_t complete=0;
    while(complete<ps.size()) {
      std::vector<int> ready; for(size_t i=0;i<ps.size();++i) if(!queued[i]&&rem[i]>0&&ps[i].arrival<=t) ready.push_back((int)i);
      std::sort(ready.begin(),ready.end(),[&](int a,int b){return std::tie(ps[a].arrival,a)<std::tie(ps[b].arrival,b);});
      for(int i:ready){q.push_back(i);queued[i]=true;}
      if(q.empty()){int next=1000000000;for(size_t i=0;i<ps.size();++i)if(rem[i]>0)next=std::min(next,ps[i].arrival);t=next;continue;}
      int i=q.front();q.erase(q.begin());int slice=std::min(quantum,rem[i]);t+=slice;rem[i]-=slice;
      ready.clear();for(size_t j=0;j<ps.size();++j)if(!queued[j]&&rem[j]>0&&ps[j].arrival<=t)ready.push_back((int)j);
      std::sort(ready.begin(),ready.end(),[&](int a,int b){return std::tie(ps[a].arrival,a)<std::tie(ps[b].arrival,b);});for(int j:ready){q.push_back(j);queued[j]=true;}
      if(rem[i]>0)q.push_back(i);else{done[i]=t;++complete;}
    }
    std::vector<Row> out;for(size_t i=0;i<ps.size();++i)out.push_back({ps[i].pid,ps[i].arrival,ps[i].burst,ps[i].priority,done[i]-ps[i].arrival-ps[i].burst,done[i]-ps[i].arrival,done[i]});return out;
}
static double estimate(double turnaround, double burst) { // normalized software estimate, arbitrary units
    const double cpu_w=0.65, memory_w=0.20, io_w=0.15;
    return std::max(0.0, burst*(cpu_w+memory_w*0.35)+turnaround*io_w*0.25);
}
#ifndef _WIN32
static Resource snapshot() {
 Resource r; std::ifstream stat("/proc/stat"); std::string tag; long long u,n,s,id,iw,irq,si,st; if(stat>>tag>>u>>n>>s>>id>>iw>>irq>>si>>st){long long idle=id+iw,total=u+n+s+id+iw+irq+si+st; std::this_thread::sleep_for(std::chrono::milliseconds(120));stat.clear();stat.seekg(0);stat>>tag>>u>>n>>s>>id>>iw>>irq>>si>>st;long long idle2=id+iw,total2=u+n+s+id+iw+irq+si+st;r.cpu=100.0*(1.0-(double)(idle2-idle)/(total2-total));}
 long long mem=0,avail=0;std::ifstream mi("/proc/meminfo");while(mi>>tag){long long val;std::string unit;mi>>val>>unit;if(tag=="MemTotal:")mem=val;if(tag=="MemAvailable:")avail=val;}if(mem)r.memory=100.0*(mem-avail)/mem;
 std::ifstream ds("/proc/diskstats");std::string line;while(std::getline(ds,line)){std::istringstream ss(line);int major,minor;std::string dev;long long x[11]{};if(!(ss>>major>>minor>>dev))continue;for(auto&v:x)ss>>v;if(dev.rfind("loop",0)!=0&&dev.rfind("ram",0)!=0){r.read+=x[2]*512;r.write+=x[6]*512;}}
 DIR*d=opendir("/proc");if(d){while(auto*e=readdir(d)){if(std::isdigit((unsigned char)e->d_name[0]))++r.processes;}closedir(d);}return r;
}
#else
static unsigned long long ticks(FILETIME t) { ULARGE_INTEGER value{}; value.LowPart=t.dwLowDateTime; value.HighPart=t.dwHighDateTime; return value.QuadPart; }
static Resource snapshot() {
 Resource r;
 FILETIME idle1{},kernel1{},user1{},idle2{},kernel2{},user2{};
 if(GetSystemTimes(&idle1,&kernel1,&user1)) {
   Sleep(120);
   if(GetSystemTimes(&idle2,&kernel2,&user2)) {
     const auto idle=ticks(idle2)-ticks(idle1);
     const auto total=(ticks(kernel2)-ticks(kernel1))+(ticks(user2)-ticks(user1));
     if(total>0) r.cpu=100.0*(1.0-(double)idle/total);
   }
 }
 MEMORYSTATUSEX m{};m.dwLength=sizeof(m);if(GlobalMemoryStatusEx(&m))r.memory=m.dwMemoryLoad;
 HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(h!=INVALID_HANDLE_VALUE){PROCESSENTRY32 e{};e.dwSize=sizeof(e);if(Process32First(h,&e))do{++r.processes;}while(Process32Next(h,&e));CloseHandle(h);}
 return r;
}
#endif
int main(int argc,char**argv){try{
 std::string input=argc>1?argv[1]:"data/workload.csv";auto ps=workload(input);std::vector<std::pair<std::string,std::vector<Row>>> all;
 all.push_back({"FCFS",nonpreemptive(ps,0)});all.push_back({"SJF",nonpreemptive(ps,1)});all.push_back({"Priority",nonpreemptive(ps,2)});all.push_back({"Round Robin",roundRobin(ps,2)});
 // Energy-aware: choose the lowest estimated resource cost. Priority and pid provide
 // deterministic memory / I/O load proxies because this workload has no such columns.
 std::vector<int> order;std::vector<bool> used(ps.size(),false);double clock=0;
 while(order.size()<ps.size()){int best=-1;double score=1e99;for(size_t i=0;i<ps.size();++i)if(!used[i]&&ps[i].arrival<=clock){double e=estimate(ps[i].burst,ps[i].burst)+0.15*ps[i].priority+0.12*(ps[i].pid%3);if(e<score||(e==score&&(best<0||ps[i].burst<ps[best].burst))){best=(int)i;score=e;}}if(best<0){int next=1e9;for(size_t i=0;i<ps.size();++i)if(!used[i])next=std::min(next,ps[i].arrival);clock=next;continue;}order.push_back(best);used[best]=true;clock+=ps[best].burst;}
 all.push_back({"Energy-Aware",finish(ps,order)});
 std::ofstream out("data/scheduling_results.csv");out<<"run_id,algorithm,pid,arrival_time,burst_time,priority,waiting_time,turnaround_time,completion_time,estimated_energy\n";auto now=std::chrono::system_clock::now().time_since_epoch().count();std::string run=std::to_string(now);
 out<<std::fixed<<std::setprecision(3);for(auto& a:all)for(auto&r:a.second)out<<run<<','<<a.first<<','<<r.pid<<','<<r.arrival<<','<<r.burst<<','<<r.priority<<','<<r.waiting<<','<<r.turnaround<<','<<r.completion<<','<<estimate(r.turnaround,r.burst)<<'\n';
 Resource res=snapshot();std::ofstream ro("data/resource_logs.csv");ro<<"cpu_percent,memory_percent,disk_read_bytes,disk_write_bytes,active_processes\n"<<res.cpu<<','<<res.memory<<','<<res.read<<','<<res.write<<','<<res.processes<<'\n';
 std::cout<<"Wrote "<<all.size()<<" algorithms x "<<ps.size()<<" processes (run "<<run<<")\n";std::cout<<"Resource snapshot: CPU "<<res.cpu<<"%, memory "<<res.memory<<"%, processes "<<res.processes<<"\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}



