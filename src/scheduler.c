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
#include "include/scheduler.h"
#include "include/state.h"
#include "include/process.h"
#include "include/dispatcher.h"
#include "include/ui.h"

void fcfs_scheduler() {
    reset_processes();
    curr_time = 0;
    int completed = 0;

    while (completed < process_counter) {
        int idx = -1;
        int earliest_arrival = __INT_MAX__;
        
        for (int i = 0; i < process_counter; i++) {
            if (!processes[i].completed && processes[i].arrival_time <= curr_time) {
                if (processes[i].arrival_time < earliest_arrival) {
                    earliest_arrival = processes[i].arrival_time;
                    idx = i;
                } else if (processes[i].arrival_time == earliest_arrival && idx != -1) {
                    if (i < idx) idx = i;
                }
            }
        }

        if (idx == -1) {
            int next_arrival = __INT_MAX__;
            for (int i = 0; i < process_counter; i++) {
                if (!processes[i].completed && processes[i].arrival_time > curr_time) {
                    if (processes[i].arrival_time < next_arrival) {
                        next_arrival = processes[i].arrival_time;
                    }
                }
            }
            if (next_arrival != __INT_MAX__) {
                simulate_idle(next_arrival - curr_time, curr_time, "FCFS");
                curr_time = next_arrival;
            }
            continue;
        }

        if (processes[idx].start_time == -1) {
            processes[idx].start_time = curr_time;
        }
        processes[idx].waiting_time = curr_time - processes[idx].arrival_time;
        
        int start = curr_time;
        int end = curr_time + processes[idx].burst_time;
        
        simulate_execution(idx, processes[idx].burst_time, start, "FCFS");
        
        processes[idx].end_time = end;
        processes[idx].turnaround_time = processes[idx].end_time - processes[idx].arrival_time;
        processes[idx].completed = 1;
        curr_time = end;
        completed++;
    }

    printf("\033[H\033[J");
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : FCFS\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    double total_waiting_time = 0;
    for (int i = 0; i < process_counter; i++) {
        total_waiting_time += processes[i].waiting_time;
    }
    printf("   └─ Average Waiting Time : %.2f time units\n", total_waiting_time / process_counter);
    print_gantt(curr_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
    if(step_mode) { printf("Press ENTER to continue..."); int c; while((c = getchar()) != '\n' && c != EOF); }
}

void sjf_scheduler() {
    reset_processes();
    curr_time = 0;
    int completed = 0;

    while (completed < process_counter) {
        int idx = -1;
        int min_burst = __INT_MAX__;
        
        for (int i = 0; i < process_counter; i++) {
            if (!processes[i].completed && processes[i].arrival_time <= curr_time) {
                if (processes[i].burst_time < min_burst) {
                    min_burst = processes[i].burst_time;
                    idx = i;
                } else if (processes[i].burst_time == min_burst && idx != -1) {
                    if (processes[i].arrival_time < processes[idx].arrival_time) {
                        idx = i;
                    } else if (processes[i].arrival_time == processes[idx].arrival_time) {
                        if (i < idx) idx = i;
                    }
                }
            }
        }

        if (idx == -1) {
            int next_arrival = __INT_MAX__;
            for (int i = 0; i < process_counter; i++) {
                if (!processes[i].completed && processes[i].arrival_time > curr_time) {
                    if (processes[i].arrival_time < next_arrival) {
                        next_arrival = processes[i].arrival_time;
                    }
                }
            }
            if (next_arrival != __INT_MAX__) {
                simulate_idle(next_arrival - curr_time, curr_time, "SJF");
                curr_time = next_arrival;
            }
            continue;
        }

        if (processes[idx].start_time == -1) {
            processes[idx].start_time = curr_time;
        }
        processes[idx].waiting_time = curr_time - processes[idx].arrival_time;
        
        int start = curr_time;
        int end = curr_time + processes[idx].burst_time;
        
        simulate_execution(idx, processes[idx].burst_time, start, "SJF");
        
        processes[idx].end_time = end;
        processes[idx].turnaround_time = processes[idx].end_time - processes[idx].arrival_time;
        processes[idx].completed = 1;
        curr_time = end;
        completed++;
    }

    printf("\033[H\033[J");
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : SJF\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    double total_waiting_time = 0;
    for (int i = 0; i < process_counter; i++) {
        total_waiting_time += processes[i].waiting_time;
    }
    printf("   └─ Average Waiting Time : %.2f time units\n", total_waiting_time / process_counter);
    print_gantt(curr_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
    if(step_mode) { printf("Press ENTER to continue..."); int c; while((c = getchar()) != '\n' && c != EOF); }
}

void priority_scheduler() {
    reset_processes();
    curr_time = 0;
    int completed = 0;

    while (completed < process_counter) {
        int idx = -1;
        int highest_priority = __INT_MAX__;
        
        for (int i = 0; i < process_counter; i++) {
            if (!processes[i].completed && processes[i].arrival_time <= curr_time) {
                if (processes[i].priority < highest_priority) {
                    highest_priority = processes[i].priority;
                    idx = i;
                } else if (processes[i].priority == highest_priority && idx != -1) {
                    if (processes[i].arrival_time < processes[idx].arrival_time) {
                        idx = i;
                    } else if (processes[i].arrival_time == processes[idx].arrival_time) {
                        if (i < idx) idx = i;
                    }
                }
            }
        }

        if (idx == -1) {
            int next_arrival = __INT_MAX__;
            for (int i = 0; i < process_counter; i++) {
                if (!processes[i].completed && processes[i].arrival_time > curr_time) {
                    if (processes[i].arrival_time < next_arrival) {
                        next_arrival = processes[i].arrival_time;
                    }
                }
            }
            if (next_arrival != __INT_MAX__) {
                simulate_idle(next_arrival - curr_time, curr_time, "Priority");
                curr_time = next_arrival;
            }
            continue;
        }

        if (processes[idx].start_time == -1) {
            processes[idx].start_time = curr_time;
        }
        processes[idx].waiting_time = curr_time - processes[idx].arrival_time;
        
        int start = curr_time;
        int end = curr_time + processes[idx].burst_time;
        
        simulate_execution(idx, processes[idx].burst_time, start, "Priority");
        
        processes[idx].end_time = end;
        processes[idx].turnaround_time = processes[idx].end_time - processes[idx].arrival_time;
        processes[idx].completed = 1;
        curr_time = end;
        completed++;
    }

    printf("\033[H\033[J");
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Priority\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    double total_waiting_time = 0;
    for (int i = 0; i < process_counter; i++) {
        total_waiting_time += processes[i].waiting_time;
    }
    printf("   └─ Average Waiting Time : %.2f time units\n", total_waiting_time / process_counter);
    print_gantt(curr_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
    if(step_mode) { printf("Press ENTER to continue..."); int c; while((c = getchar()) != '\n' && c != EOF); }
}

void round_robin_scheduler() {
    reset_processes();
    int current_time = 0;
    int completed_processes = 0;
    
    struct {
        int data[MAX_PROCESSES_NUM * 2]; 
        int start;
        int end;
        int size;
    } ready_queue = {0};
    
    int process_in_queue[MAX_PROCESSES_NUM] = {0};
    
    int min_arrival_time = processes[0].arrival_time;
    for (int proc = 1; proc < process_counter; proc++) {
        if (processes[proc].arrival_time < min_arrival_time) {
            min_arrival_time = processes[proc].arrival_time;
        }
    }
    
    if (current_time < min_arrival_time) {
        simulate_idle(min_arrival_time - current_time, current_time, "Round Robin");
        current_time = min_arrival_time;
    }
    
    for (int proc = 0; proc < process_counter; proc++) {
        if (processes[proc].arrival_time <= current_time && !process_in_queue[proc]) {
            ready_queue.data[ready_queue.end] = proc;
            ready_queue.end = (ready_queue.end + 1) % (MAX_PROCESSES_NUM * 2);
            ready_queue.size++;
            process_in_queue[proc] = 1;
        }
    }
    
    while (completed_processes < process_counter) {
        if (ready_queue.size == 0) {
            int next_process_time = -1;
            for (int proc = 0; proc < process_counter; proc++) {
                if (!processes[proc].completed && processes[proc].arrival_time > current_time) {
                    if (next_process_time == -1 || processes[proc].arrival_time < next_process_time) {
                        next_process_time = processes[proc].arrival_time;
                    }
                }
            }
            if (next_process_time != -1) {
                simulate_idle(next_process_time - current_time, current_time, "Round Robin");
                current_time = next_process_time;
                for (int proc = 0; proc < process_counter; proc++) {
                    if (!process_in_queue[proc] && processes[proc].arrival_time <= current_time) {
                        ready_queue.data[ready_queue.end] = proc;
                        ready_queue.end = (ready_queue.end + 1) % (MAX_PROCESSES_NUM * 2);
                        ready_queue.size++;
                        process_in_queue[proc] = 1;
                    }
                }
                continue;
            }
        }
        
        int process_index = ready_queue.data[ready_queue.start];
        ready_queue.start = (ready_queue.start + 1) % (MAX_PROCESSES_NUM * 2);
        ready_queue.size--;
        
        Process *current_process = &processes[process_index];
        if (current_process->remaining_time <= 0) continue;
        
        if (current_process->start_time == -1) {
            current_process->start_time = current_time;
        }
        
        int time_slice = (current_process->remaining_time < time_quantum) ? 
                        current_process->remaining_time : time_quantum;
        
        simulate_execution(process_index, time_slice, current_time, "Round Robin");
        current_time += time_slice;
        current_process->remaining_time -= time_slice;
        
        for (int proc = 0; proc < process_counter; proc++) {
            if (!process_in_queue[proc] && 
                processes[proc].arrival_time <= current_time && 
                processes[proc].arrival_time > (current_time - time_slice)) {
                ready_queue.data[ready_queue.end] = proc;
                ready_queue.end = (ready_queue.end + 1) % (MAX_PROCESSES_NUM * 2);
                ready_queue.size++;
                process_in_queue[proc] = 1;
            }
        }
        
        if (current_process->remaining_time > 0) {
            ready_queue.data[ready_queue.end] = process_index;
            ready_queue.end = (ready_queue.end + 1) % (MAX_PROCESSES_NUM * 2);
            ready_queue.size++;
        } else {
            current_process->end_time = current_time;
            current_process->turnaround_time = current_process->end_time - current_process->arrival_time;
            current_process->waiting_time = current_process->turnaround_time - current_process->burst_time;
            current_process->completed = 1;
            completed_processes++;
        }
    }
    
    printf("\033[H\033[J");
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Round Robin\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    double total_waiting_time = 0;
    for (int i = 0; i < process_counter; i++) {
        total_waiting_time += processes[i].waiting_time;
    }
    printf("   └─ Average Waiting Time : %.2f time units\n", total_waiting_time / process_counter);
    print_gantt(current_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
    if(step_mode) { printf("Press ENTER to continue..."); int c; while((c = getchar()) != '\n' && c != EOF); }
}
