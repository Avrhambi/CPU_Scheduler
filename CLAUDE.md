# CLAUDE.md

## What this is
A POSIX CPU-scheduler simulator in C. It runs one **real child process** per
simulated process and drives them with `SIGSTOP`/`SIGCONT`, so context switching is
performed by the kernel rather than mocked. Implements FCFS, SJF, Priority, and
Round Robin; renders a live ANSI dashboard and a per-process Gantt chart.

## Run / build
- Build: `make` (produces `./CPU-Scheduler`; `-Wall -Wextra`). `make clean` to reset.
- Demo run: `./CPU-Scheduler --demo --step` (3–5 random processes, tick-by-tick).
- CSV run: `./CPU-Scheduler <file.csv> <rr_quantum>` — CSV columns are
  `name,description,arrival_time,burst_time,priority`.
- **UNIX only** (`fork`, POSIX signals, `waitpid`). On Windows use WSL.
- No test suite or CI yet — a clean `make` is the only mechanical gate.

## Key files
- `src/main.c` — CLI parsing (`getopt_long`), installs `SIGALRM` handler, runs the
  four schedulers in sequence, sweeps children between runs.
- `src/scheduler.c` — the four algorithms; each a discrete-time loop over the global clock.
- `src/dispatcher.c` — `fork`/`SIGCONT`/`SIGSTOP` dispatch and the `SIGALRM` +
  `sigsuspend` 1 Hz tick (`simulate_execution` / `simulate_idle`).
- `src/process.c` — `parse_csv`, `generate_demo_processes`, `fork`, `SIGTERM` cleanup.
- `src/ui.c` — `print_dashboard` (ANSI in-place redraw), `print_gantt`.
- `src/state.c` / `include/state.h` — shared mutable state (`processes[]`, clock,
  `gantt_history[]`, mode flags). `include/types.h` — `Process` struct + capacities.

## Architecture notes
- Discrete-time simulation: one "tick" = one real second (or one keypress in
  `--step`). The scheduler picks a process, the dispatcher runs it for N ticks.
- Child processes only `pause()` in a loop — they exist so the kernel owns their
  run/stop state. No `execve`.
- The `SIGALRM` clock is made race-free by blocking the signal, checking
  `alarm_triggered` (`volatile sig_atomic_t`), then `sigsuspend`.
- All state is statically pre-allocated — no `malloc`/`free`. Fixed ceilings:
  `MAX_PROCESSES_NUM` 1000, `gantt_history` 10000 ticks (bounds-checked).
- `main` runs all four algorithms per invocation, calling `processes_cleaning`
  (`SIGCONT`+`SIGTERM`+`waitpid`) between each so every run starts clean.
