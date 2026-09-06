#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

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

Process processes[MAX_PROCESSES_NUM];
int process_counter = 0;
int curr_time = 0;
int time_quantum = 0;
volatile sig_atomic_t alarm_triggered = 0;

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

int check_pending_signals() {
    sigset_t pending;
    
    if (sigpending(&pending) == -1) {;
        return 0;
    }
    return sigismember(&pending, SIGALRM);
}

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
        kill(pid, SIGTERM);
        int status;
        waitpid(pid, &status, 0);
    }
}

void simulate_execution(int process_idx, int duration) {
    if (duration <= 0) return;
    
    Process *p = &processes[process_idx];
    
    
    if (p->pid == -1) {
        p->pid = create_process();
    }
    
    
    alarm_triggered = 0;
    alarm(duration);
    
    sigset_t oldset;
    block_signals(&oldset);
    
    while (!alarm_triggered) {
        sigsuspend(&oldset);
    }
    
    unblock_signals(&oldset);
    terminate_process(p->pid);
    p->pid = -1;
}

void fcfs_scheduler() {
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : FCFS\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

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
                    if (i < idx) {
                        idx = i;
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
                printf("%d → %d: Idle.\n", curr_time, next_arrival);
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
        
        printf("%d → %d: %s Running %s.\n", start, end, processes[idx].name, processes[idx].desc);
        
        simulate_execution(idx, processes[idx].burst_time);
        
        processes[idx].end_time = end;
        processes[idx].turnaround_time = processes[idx].end_time - processes[idx].arrival_time;
        processes[idx].completed = 1;
        curr_time = end;
        completed++;
    }

    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");

    double total_waiting_time = 0;
    for (int i = 0; i < process_counter; i++) {
        total_waiting_time += processes[i].waiting_time;
    }
    printf("   └─ Average Waiting Time : %.2f time units\n", total_waiting_time / process_counter);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}

void sjf_scheduler() {
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : SJF\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

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
                        if (i < idx) {
                            idx = i;
                        }
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
                printf("%d → %d: Idle.\n", curr_time, next_arrival);
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
        
        printf("%d → %d: %s Running %s.\n", start, end, processes[idx].name, processes[idx].desc);
        
        simulate_execution(idx, processes[idx].burst_time);
        
        processes[idx].end_time = end;
        processes[idx].turnaround_time = processes[idx].end_time - processes[idx].arrival_time;
        processes[idx].completed = 1;
        curr_time = end;
        completed++;
    }

    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");

    double total_waiting_time = 0;
    for (int i = 0; i < process_counter; i++) {
        total_waiting_time += processes[i].waiting_time;
    }
    printf("   └─ Average Waiting Time : %.2f time units\n", total_waiting_time / process_counter);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}

void priority_scheduler() {
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Priority\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

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
                        if (i < idx) {
                            idx = i;
                        }
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
                printf("%d → %d: Idle.\n", curr_time, next_arrival);
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
        
        printf("%d → %d: %s Running %s.\n", start, end, processes[idx].name, processes[idx].desc);
        
        simulate_execution(idx, processes[idx].burst_time);
        
        processes[idx].end_time = end;
        processes[idx].turnaround_time = processes[idx].end_time - processes[idx].arrival_time;
        processes[idx].completed = 1;
        curr_time = end;
        completed++;
    }

    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");

    double total_waiting_time = 0;
    for (int i = 0; i < process_counter; i++) {
        total_waiting_time += processes[i].waiting_time;
    }
    printf("   └─ Average Waiting Time : %.2f time units\n", total_waiting_time / process_counter);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}

void round_rubin_scheduler() {
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Round Robin\n>> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

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
        printf("%d → %d: Idle.\n", current_time, min_arrival_time);
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
                printf("%d → %d: Idle.\n", current_time, next_process_time);
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
        
        
        printf("%d → %d: %s Running %s.\n", 
               current_time, current_time + time_slice, 
               current_process->name, current_process->desc);
        
        simulate_execution(process_index, time_slice);
        
        current_time += time_slice;
        current_process->remaining_time -= time_slice;
        
        
        for (int proc = 0; proc < process_counter; proc++) {
            if (!process_in_queue[proc] && 
                processes[proc].arrival_time < current_time && 
                processes[proc].arrival_time >= (current_time - time_slice)) {
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
        
        
        for (int proc = 0; proc < process_counter; proc++) {
            if (!process_in_queue[proc] && processes[proc].arrival_time == current_time) {
                ready_queue.data[ready_queue.end] = proc;
                ready_queue.end = (ready_queue.end + 1) % (MAX_PROCESSES_NUM * 2);
                ready_queue.size++;
                process_in_queue[proc] = 1;
            }
        }
    }
    
    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Total Turnaround Time : %d time units\n", current_time);
    printf("\n>> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}

void processes_cleaning() {
    for (int i = 0; i < process_counter; i++) {
        if (processes[i].pid > 0) {
            terminate_process(processes[i].pid);
            processes[i].pid = -1;
        }
    }
}

void runCPUScheduler(char* filename, int quantum) {
    time_quantum = quantum;
    setup_cpu_signal_handlers();
    parse_csv(filename);
    
    fcfs_scheduler();
    processes_cleaning();
    
    sjf_scheduler();
    processes_cleaning();
    
    priority_scheduler();
    processes_cleaning();
    
    round_rubin_scheduler();
    processes_cleaning();
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <csv_filename> <quantum>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char *filename = argv[1];
    int quantum = atoi(argv[2]);
    
    if (quantum <= 0) {
        fprintf(stderr, "Error: Quantum must be a positive integer.\\n");
        return EXIT_FAILURE;
    }
    
    runCPUScheduler(filename, quantum);
    
    return EXIT_SUCCESS;
}
