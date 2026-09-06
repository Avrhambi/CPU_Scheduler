#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <signal.h>

void cpu_alarm_handler(int signum);
void setup_cpu_signal_handlers();
void block_signals(sigset_t *oldset);
void unblock_signals(sigset_t *oldset);
void wait_for_step();
void simulate_idle(int duration, int start_t, const char* algo_name);
void simulate_execution(int process_idx, int duration, int start_t, const char* algo_name);

#endif
