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
#include "include/ui.h"
#include "include/state.h"

void print_dashboard(int running_idx, int t, const char* algo_name) {
    printf("\033[H\033[J"); // Clear screen
    
    printf("══════════════════════════════════════════════\n");
    printf(">> Process Information\n");
    printf("──────────────────────────────────────────────\n");
    printf("%-10s %-15s %-15s %-15s\n", "Process", "Arrival Time", "Burst Time", "Priority");
    int total_burst = 0;
    for (int i = 0; i < process_counter; i++) {
        printf("%-10s %-15d %-15d %-15d\n", 
               processes[i].name, 
               processes[i].arrival_time, 
               processes[i].burst_time, 
               processes[i].priority);
        total_burst += processes[i].burst_time;
    }
    printf("──────────────────────────────────────────────\n");
    printf("%-26s %-15d\n", "Total Burst Time:", total_burst);

    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : %s\n", algo_name);
    printf(">> Current Time   : %d\n", t);
    printf("──────────────────────────────────────────────\n");
    
    if (running_idx != -1) {
        printf(">> [RUNNING]      : %s (%s)\n", processes[running_idx].name, processes[running_idx].desc);
    } else {
        printf(">> [RUNNING]      : IDLE\n");
    }
    
    printf(">> [READY QUEUE]  : ");
    int first = 1;
    for (int i = 0; i < process_counter; i++) {
        if (i != running_idx && !processes[i].completed && processes[i].arrival_time <= t) {
            if (!first) printf(", ");
            printf("%s", processes[i].name);
            first = 0;
        }
    }
    if (first) printf("(empty)");
    printf("\n══════════════════════════════════════════════\n");
}

void print_gantt(int total_time) {
    printf("\n>> Gantt Chart:\n");
    for (int i = 0; i < process_counter; i++) {
        printf("   %-10s: [", processes[i].name);
        for (int t = 0; t < total_time && t < 10000; t++) {
            if (gantt_history[t] == i) printf("█");
            else printf(" ");
        }
        printf("]\n");
    }
}
