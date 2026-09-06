#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <stdbool.h>
#include "include/dispatcher.h"
#include "include/state.h"
#include "include/process.h"
#include "include/ui.h"

void cpu_alarm_handler(int signum) {
    alarm_triggered = 1;
}

void setup_cpu_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = cpu_alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        exit(EXIT_FAILURE);
    }
}

void block_signals(sigset_t *oldset) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &set, oldset) == -1) {
        exit(EXIT_FAILURE);
    }
}

void unblock_signals(sigset_t *oldset) {
    if (sigprocmask(SIG_SETMASK, oldset, NULL) == -1) {
        exit(EXIT_FAILURE);
    }
}

void wait_for_step() {
    if (step_mode) {
        printf("\nPress ENTER to continue...");
        fflush(stdout);
        int c;
        while((c = getchar()) != '\n' && c != EOF);
    } else {
        alarm_triggered = 0;
        alarm(1);
        sigset_t oldset;
        block_signals(&oldset);
        while (!alarm_triggered) {
            sigsuspend(&oldset);
        }
        unblock_signals(&oldset);
    }
}

void simulate_idle(int duration, int start_t, const char* algo_name) {
    for (int step = 0; step < duration; step++) {
        int t = start_t + step;
        if(t < 10000) gantt_history[t] = -1;
        print_dashboard(-1, t, algo_name);
        wait_for_step();
    }
}

void simulate_execution(int process_idx, int duration, int start_t, const char* algo_name) {
    if (duration <= 0) return;
    Process *p = &processes[process_idx];
    
    if (p->pid == -1) {
        p->pid = create_process();
    } else {
        kill(p->pid, SIGCONT);
    }
    
    for (int step = 0; step < duration; step++) {
        int t = start_t + step;
        if(t < 10000) gantt_history[t] = process_idx;
        print_dashboard(process_idx, t, algo_name);
        wait_for_step();
    }
    kill(p->pid, SIGSTOP);
}
