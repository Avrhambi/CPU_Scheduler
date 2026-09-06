#ifndef TYPES_H
#define TYPES_H

#include <sys/types.h>

#define MAX_PROCESSES_NUM 1000
#define MAX_NAME_LEN 51
#define MAX_DESC_LEN 101

typedef struct {
    char name[MAX_NAME_LEN];
    char desc[MAX_DESC_LEN];
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_time;
    pid_t pid;
    int start_time;
    int end_time;
    int waiting_time;
    int turnaround_time;
    int completed;
} Process;

#endif
