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
#include "include/process.h"
#include "include/state.h"
#include "include/ui.h"

void parse_csv(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        exit(EXIT_FAILURE);
    }

    char line[256];
    process_counter = 0;

    while (fgets(line, sizeof(line), file) && process_counter < MAX_PROCESSES_NUM) {
        line[strcspn(line, "\n")] = 0;
        
        char* token = strtok(line, ",");
        if (!token) continue;
        strncpy(processes[process_counter].name, token, MAX_NAME_LEN - 1);
        processes[process_counter].name[MAX_NAME_LEN - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(processes[process_counter].desc, token, MAX_DESC_LEN - 1);
        processes[process_counter].desc[MAX_DESC_LEN - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        processes[process_counter].arrival_time = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        processes[process_counter].burst_time = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        processes[process_counter].priority = atoi(token);

        processes[process_counter].remaining_time = processes[process_counter].burst_time;
        processes[process_counter].pid = -1;
        processes[process_counter].start_time = -1;
        processes[process_counter].end_time = -1;
        processes[process_counter].waiting_time = 0;
        processes[process_counter].turnaround_time = 0;
        processes[process_counter].completed = 0;

        process_counter++;
    }
    fclose(file);
}

void generate_demo_processes() {
    srand(time(NULL));
    process_counter = 3 + rand() % 3; // 3 to 5 processes
    for(int i=0; i<process_counter; i++) {
        sprintf(processes[i].name, "P%d", i+1);
        sprintf(processes[i].desc, "Demo Process %d", i+1);
        processes[i].arrival_time = rand() % 5;
        processes[i].burst_time = 1 + rand() % 5;
        processes[i].priority = 1 + rand() % 5;
        
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].pid = -1;
        processes[i].start_time = -1;
        processes[i].end_time = -1;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].completed = 0;
    }

    // Removed manual print, now handled by the dashboard
}

void reset_processes() {
    for (int i = 0; i < process_counter; i++) {
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].pid = -1;
        processes[i].start_time = -1;
        processes[i].end_time = -1;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].completed = 0;
    }
    for(int i = 0; i < 10000; i++) {
        gantt_history[i] = -1;
    }
}

pid_t create_process() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        while (1) {
            pause();
        }
        exit(0);
    }
    return pid; 
}

void terminate_process(pid_t pid) {
    if (pid > 0) {
        kill(pid, SIGCONT);
        kill(pid, SIGTERM);
        int status;
        waitpid(pid, &status, 0);
    }
}

void processes_cleaning() {
    for (int i = 0; i < process_counter; i++) {
        if (processes[i].pid > 0) {
            terminate_process(processes[i].pid);
            processes[i].pid = -1;
        }
    }
}
