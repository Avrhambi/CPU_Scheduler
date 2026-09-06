# OS Process Scheduler & Signal Management Engine

> A POSIX CPU-scheduler simulator that runs **real child processes** and drives them
> with `SIGSTOP` / `SIGCONT`, so context switching is performed by the kernel rather
> than mocked with threads. Renders a live ANSI dashboard and a per-process Gantt
> chart for FCFS, SJF, Priority, and Round Robin.

## System Architecture & Flow

The program is a single-process orchestrator that runs each algorithm in turn
against a shared job pool, spawning one real child process per simulated process.

**Components**

| Module | Role |
| --- | --- |
| `src/main.c` | CLI parsing (`getopt_long`), installs the `SIGALRM` handler, loads processes, runs the four schedulers back-to-back, sweeps children between runs. |
| `src/scheduler.c` | The four algorithms (FCFS, SJF, Priority, Round Robin). Each is a discrete-time loop over the global clock that picks the next process and asks the dispatcher to run it for N ticks. |
| `src/dispatcher.c` | Turns a scheduling decision into OS actions: `fork` the child on first run, `SIGCONT` to resume, `SIGSTOP` to preempt, and one `SIGALRM`-driven 1-second wall-clock tick per unit of simulated time. |
| `src/process.c` | Process sourcing and lifecycle: CSV parsing, random demo generation, `fork`, and `SIGTERM` teardown with `waitpid` reaping. |
| `src/ui.c` | All rendering: the in-place ANSI dashboard (`[RUNNING]` / `[READY QUEUE]`) and the end-of-run Gantt chart. |
| `src/state.c` | The shared mutable state — job pool (`processes[]`), global clock, `gantt_history[]`, mode flags — declared once, referenced everywhere. |

**One simulated time unit (tick)**

1. **Select.** The active algorithm scans the job pool for processes that have
   arrived and aren't complete, and picks one by its rule (arrival order / shortest
   burst / lowest priority number / round-robin front). If nothing has arrived yet,
   it fast-forwards the clock to the next arrival and marks the gap idle.
2. **Dispatch.** The dispatcher `fork`s the child on its first slice, otherwise
   sends `SIGCONT`. The child does nothing but `pause()` in a loop — it exists so
   the kernel, not the program, owns its run/stop state.
3. **Render.** `print_dashboard` clears the screen with ANSI escapes and repaints
   the process table, the running process, and the ready queue for the current tick.
4. **Advance.** `gantt_history[t]` records who ran at tick `t`; the program then
   blocks for one real second on `sigsuspend` until `SIGALRM` fires (or for a
   keypress in `--step` mode).
5. **Preempt / complete.** When the slice ends (burst finished, or Round Robin
   quantum expired) the dispatcher sends `SIGSTOP`. A preempted process goes back
   to the queue; a finished one has its turnaround/waiting times recorded.
6. **Report.** After all processes complete, the algorithm prints the average
   waiting time and the Gantt chart, then (in `--step` mode) waits for ENTER before
   the next algorithm.
7. **Sweep.** `main` calls `processes_cleaning` between algorithms: `SIGCONT` +
   `SIGTERM` + `waitpid` on every child so no zombies or stopped processes leak.

## Tech Stack & Engineering Decisions

| Layer | Technology | Rationale & trade-offs |
| --- | --- | --- |
| Core engine | C (C99), `gcc -Wall -Wextra` | Direct access to `fork`, signals, and `waitpid`; the simulation *is* the OS primitives. Trade-off: manual lifecycle management, no memory safety net. |
| Context switching | POSIX `SIGSTOP` / `SIGCONT` on real children | Real kernel-level stop/resume instead of cooperative threads or a state enum — the preemption is genuine. Trade-off: one `fork` per process, and every signal path needs care to avoid races. |
| Clock | `SIGALRM` + `sigsuspend` with `SIGALRM` blocked | Race-free 1 Hz tick: the signal is masked, the flag is checked, then `sigsuspend` atomically unblocks-and-waits. Trade-off: real-time pacing makes a long simulation slow to watch (mitigated by `--step`). |
| UI | Raw ANSI escape codes | Zero dependencies, in-place redraw without `ncurses`. Trade-off: assumes an ANSI-capable terminal; no resize handling. |
| Memory | Static pre-allocation (`processes[1000]`, `gantt_history[10000]`) | No `malloc`/`free`, so no leak or fragmentation surface. Trade-off: fixed ceilings (1000 processes, 10000 ticks) enforced by bounds checks. |
| Input | `getopt_long` + CSV (`name,desc,arrival,burst,priority`) | Standard flag parsing; CSV is diff-friendly and trivial to hand-edit. `--demo` generates 3–5 random processes for a zero-setup run. |

## Resilience & Error Handling Patterns

- **Signal-safe clock.** `dispatcher.c` blocks `SIGALRM` with `sigprocmask`, checks
  `alarm_triggered` (a `volatile sig_atomic_t`), and only then calls `sigsuspend` —
  the handler cannot fire in the window between test and wait, so no tick is lost or
  double-counted.
- **Bounded history buffer.** Every write to `gantt_history[t]` and every Gantt
  render is guarded by `t < 10000`, so an unusually long schedule truncates the
  chart instead of overrunning the array.
- **EOF-safe stepping.** The `--step` prompt loops on `getchar()` until `'\n'`
  **or `EOF`**, so piping input or pressing `Ctrl+D` ends the wait cleanly instead
  of spinning on a closed stream.
- **Child cleanup between runs.** `processes_cleaning` runs after every algorithm:
  `SIGCONT` (so a stopped child can receive the next signal), `SIGTERM`, then a
  blocking `waitpid` to reap it. Each of the four runs starts from a clean process
  table.
- **Input validation.** `main` rejects a missing input source and a non-positive
  quantum before any process is created; `parse_csv` skips malformed rows and caps
  at `MAX_PROCESSES_NUM`.

## Project Layout

Modular by concern — algorithm logic, OS/signal handling, rendering, and state are
each isolated behind their own header.

```text
.
├── include/
│   ├── types.h        # Process struct + capacity constants
│   ├── state.h        # extern declarations for the shared simulation state
│   ├── scheduler.h    # the four scheduling algorithms
│   ├── dispatcher.h   # signal setup, dispatch, tick pacing
│   ├── process.h      # CSV/demo sourcing, fork, termination
│   └── ui.h           # dashboard + Gantt rendering
├── src/
│   ├── main.c         # CLI parsing and orchestration
│   ├── state.c        # definitions of the shared state
│   ├── scheduler.c    # FCFS, SJF, Priority, Round Robin
│   ├── dispatcher.c   # SIGSTOP/SIGCONT/SIGALRM, simulate_execution/idle
│   ├── process.c      # parse_csv, generate_demo_processes, fork, cleanup
│   └── ui.c           # print_dashboard, print_gantt
├── Makefile           # gcc build of the six translation units
└── README.md
```

## Local Setup & Quickstart

> [!IMPORTANT]
> Requires a UNIX-like environment — the engine uses `fork`, POSIX signals, and
> `waitpid`. Linux and macOS run it natively; on Windows use **WSL**. It will not
> build or run under CMD or PowerShell.

```bash
# Clone
git clone https://github.com/Avrhambi/CPU_Scheduler
cd CPU_Scheduler

# Build (produces ./CPU-Scheduler)
make

# Zero-setup run: 3-5 random processes, step through tick by tick
./CPU-Scheduler --demo --step

# Real-time run from a CSV, Round Robin quantum = 3
#   CSV columns: name,description,arrival_time,burst_time,priority
./CPU-Scheduler processes.csv 3

# Rebuild from scratch
make clean && make
```

All four algorithms run in sequence in a single invocation; each finishes with its
average waiting time and Gantt chart.

## Important Notes

The build is the only mechanical gate:

```bash
make clean && make   # must compile clean under -Wall -Wextra
```

Behaviour is currently verified by hand: `--demo --step` to walk a schedule, and
small CSV fixtures with known optimal orderings to eyeflow the Gantt output and the
reported average waiting time. A unit harness around the four schedulers (feed a
fixed process table, assert on `waiting_time` / `turnaround_time`) and a GitHub
Actions job running the build are the obvious next steps.
