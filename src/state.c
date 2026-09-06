#include "include/state.h"

Process processes[MAX_PROCESSES_NUM];
int process_counter = 0;
int curr_time = 0;
int time_quantum = 0;
volatile sig_atomic_t alarm_triggered = 0;
int demo_mode = 0;
int step_mode = 0;
int gantt_history[10000];
