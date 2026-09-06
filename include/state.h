#ifndef STATE_H
#define STATE_H

#include "types.h"
#include <signal.h>

extern Process processes[MAX_PROCESSES_NUM];
extern int process_counter;
extern int curr_time;
extern int time_quantum;
extern volatile sig_atomic_t alarm_triggered;
extern int demo_mode;
extern int step_mode;
extern int gantt_history[10000];

#endif
