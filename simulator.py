"""Discrete-event CPU scheduling simulation used by the interactive dashboard."""
from dataclasses import dataclass


@dataclass(frozen=True)
class Process:
    pid: int
    arrival: int
    burst: int
    priority: int


def energy_score(turnaround, burst):
    """Relative project estimate; this is not measured electrical energy."""
    return max(0.0, burst * (0.65 + 0.20 * 0.35) + turnaround * 0.15 * 0.25)


def simulate(process_rows, algorithm, quantum=2):
    """Return execution slices and per-process metrics for one scheduling policy."""
    processes = [Process(int(r['pid']), int(r['arrival_time']), int(r['burst_time']), int(r['priority'])) for r in process_rows]
    if not processes:
        raise ValueError('The selected workload contains no processes.')

    remaining = {p.pid: p.burst for p in processes}
    first_start = {}
    completion = {}
    slices = []
    clock = 0

    if algorithm == 'Round Robin':
        queue = []
        enqueued = set()
        while len(completion) < len(processes):
            arriving = sorted((p for p in processes if p.pid not in enqueued and remaining[p.pid] and p.arrival <= clock), key=lambda p: (p.arrival, p.pid))
            for p in arriving:
                queue.append(p); enqueued.add(p.pid)
            if not queue:
                clock = min(p.arrival for p in processes if remaining[p.pid] > 0)
                continue
            p = queue.pop(0)
            first_start.setdefault(p.pid, clock)
            end = clock + min(quantum, remaining[p.pid])
            slices.append({'pid': p.pid, 'start': clock, 'end': end})
            remaining[p.pid] -= end - clock
            clock = end
            arriving = sorted((candidate for candidate in processes if candidate.pid not in enqueued and remaining[candidate.pid] and candidate.arrival <= clock), key=lambda candidate: (candidate.arrival, candidate.pid))
            for candidate in arriving:
                queue.append(candidate); enqueued.add(candidate.pid)
            if remaining[p.pid]:
                queue.append(p)
            else:
                completion[p.pid] = clock
    else:
        if algorithm not in {'FCFS', 'SJF', 'Priority', 'Energy-Aware'}:
            raise ValueError('Choose one of the available scheduling policies.')
        done = set()
        while len(done) < len(processes):
            ready = [p for p in processes if p.pid not in done and p.arrival <= clock]
            if not ready:
                clock = min(p.arrival for p in processes if p.pid not in done)
                ready = [p for p in processes if p.pid not in done and p.arrival <= clock]
            if algorithm == 'FCFS':
                p = min(ready, key=lambda x: (x.arrival, x.pid))
            elif algorithm == 'SJF':
                p = min(ready, key=lambda x: (x.burst, x.arrival, x.pid))
            elif algorithm == 'Priority':
                p = min(ready, key=lambda x: (x.priority, x.arrival, x.pid))
            else:
                p = min(ready, key=lambda x: (energy_score(x.burst, x.burst) + 0.15 * x.priority + 0.12 * (x.pid % 3), x.burst, x.pid))
            first_start[p.pid] = clock
            end = clock + p.burst
            slices.append({'pid': p.pid, 'start': clock, 'end': end})
            completion[p.pid] = end
            done.add(p.pid)
            clock = end

    metrics = []
    for p in processes:
        turnaround = completion[p.pid] - p.arrival
        waiting = turnaround - p.burst
        metrics.append({
            'pid': p.pid, 'arrival_time': p.arrival, 'burst_time': p.burst, 'priority': p.priority,
            'first_start': first_start[p.pid], 'completion_time': completion[p.pid],
            'waiting_time': waiting, 'turnaround_time': turnaround,
            'response_time': first_start[p.pid] - p.arrival,
            'estimated_energy': round(energy_score(turnaround, p.burst), 3),
        })
    return {
        'algorithm': algorithm, 'quantum': quantum if algorithm == 'Round Robin' else None,
        'timeline': slices, 'processes': metrics,
        'averages': {
            'waiting_time': round(sum(x['waiting_time'] for x in metrics) / len(metrics), 3),
            'turnaround_time': round(sum(x['turnaround_time'] for x in metrics) / len(metrics), 3),
            'response_time': round(sum(x['response_time'] for x in metrics) / len(metrics), 3),
            'estimated_energy': round(sum(x['estimated_energy'] for x in metrics), 3),
            'throughput': round(len(metrics) / max(1, max(completion.values()) - min(p.arrival for p in processes)), 3),
        },
        'makespan': max(completion.values()),
    }
