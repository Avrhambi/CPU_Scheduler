# Portfolio Transformation Plan: OS Process Scheduler

## 🎯 The Vision
Transform the university exercise (`OS_EX3/CPU-Scheduler.c`) location: C:\Users\avrha\Documents\my_technical_knowledge\CS_Courses\שנה ג\מערכות הפעלה\תרגילים into an interactive, visual OS simulation tool named **"OS Process Scheduler & Signal Management Engine"**. This proves mastery over C, POSIX signals, concurrency, and algorithm design.

---

## 🚀 Phase 1: Extraction & Standalone Setup
*Goal: Break the code out of the "homework" structure.*
1. **Isolate the Code:** Copy `CPU-Scheduler.c` from `OS_EX3` into this folder.
2. **Decouple:** Remove any dependencies on `ex3.c` or `Focus-Mode.c`. Give the scheduler its own `int main(int argc, char *argv[])` entry point.
3. **Build System:** Create a standard `Makefile` so the project compiles cleanly with a simple `make` command.

## 🛠️ Phase 2: Productization & UX Improvements
*Goal: Make the invisible backend logic visible and user-friendly.*
1. **Add CLI Arguments (`getopt`):**
   * `--demo`: Auto-generates random processes so the user doesn't need to manually create a CSV file.
   * `--step`: Pauses the simulation after every clock tick, waiting for the user to press `ENTER`.
2. **Implement Live Dashboard:**
   * Replace basic `printf` logs with ANSI escape sequences (e.g., `\033[H\033[J` to clear the screen).
   * Draw a static "Dashboard" that updates in place, showing the `[RUNNING]` process and the `[READY QUEUE]`.
3. **Add Gantt Chart Output:**
   * At the end of the simulation, calculate turnaround and wait times.
   * Print a visual timeline using ASCII blocks (e.g., `P1: [████    ]`) to show how CPU time was distributed.

## 📚 Phase 3: Documentation
*Goal: Pass the recruiter screen.*
1. **Write `README.md`:**
   * Include the Elevator Pitch.
   * Add a GIF or screenshot of the Live Dashboard and Gantt Chart.
   * Detail the technical challenges (e.g., "How I handled async-signal-safety").
   * Highlight the scheduling algorithms implemented.
2. **Clean Code:** Run Valgrind to ensure 0 memory leaks and format the C code to industry standards (e.g., using `clang-format`).
