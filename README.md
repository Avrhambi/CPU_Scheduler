# OS Process Scheduler & Signal Management Engine

## Elevator Pitch
Welcome to the **OS Process Scheduler & Signal Management Engine**! 🚀 
This project is an advanced, full-featured simulation of a CPU scheduler, bringing the intricate world of Operating Systems to life right in your terminal. We've built an engine that perfectly balances rigorous low-level systems programming—like process state management and POSIX signal handling—with modern user experience enhancements. Watch algorithms execute in real-time and discover the power of process preemption!

## 🌟 Key Features

### Live Dashboard
Experience the scheduling process interactively with our new **Live Dashboard**. 
By running the engine in `--step` mode, you'll see a dynamically updating interface that refreshes on every clock tick. It pinpoints the current time, highlights the `[RUNNING]` process, and gives you real-time visibility into the `[READY QUEUE]`.

### Gantt Chart Visualizations
At the end of every simulation cycle, the engine prints a detailed ASCII-based **Gantt Chart** timeline (`P1: [████    ]`). This visual representation demonstrates exactly how CPU time was distributed and shared across all active processes.

### 🧠 Implemented Algorithms
The engine is equipped with classic CPU scheduling algorithms:
- **First-Come, First-Served (FCFS)**
- **Shortest Job First (SJF)**
- **Priority Scheduling**
- **Round Robin (RR)**

## 🛠️ Technical Challenges & Solutions

Developing a robust simulation came with complex technical hurdles, primarily involving concurrency and state management:

- **Handling Async-Signal-Safety:** Ensuring stability when mixing standard C functions with POSIX signal handlers was paramount. We successfully established rigorous signal masking protocols (`sigprocmask`, `sigsuspend`) to prevent deadlocks and race conditions.
- **Process Preemption using `SIGSTOP`/`SIGCONT`:** Rather than naively killing and recreating child processes during context switches, we implemented true Unix preemption. Processes are intelligently paused and resumed using POSIX signals, perfectly replicating an OS context switch.
- **Memory Bounds Management:** Using static allocations limits overhead and completely eliminates dynamic memory leaks. To prevent buffer over-reads in large simulations, we implemented tight bounds-checking dynamically across execution logs and the visual Gantt timeline.

## 🚀 Getting Started

Compile the project utilizing the provided `Makefile` or natively compile using `gcc`:
```bash
make
# OR
gcc -Wall -Wextra -g CPU-Scheduler.c -o CPU-Scheduler
```

Run in demo mode to autogenerate tasks:
```bash
./CPU-Scheduler --demo --step
```
