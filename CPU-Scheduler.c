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

int demo_mode = 0;
int step_mode = 0;
int gantt_history[10000];
void wait_for_step();

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

    printf("\n══════════════════════════════════════════════\n");
    printf(">> Generated Demo Processes\n");
    printf("──────────────────────────────────────────────\n");
    printf("%-10s %-15s %-15s %-15s\n", "Process", "Arrival Time", "Burst Time", "Priority");
    for (int i = 0; i < process_counter; i++) {
        printf("%-10s %-15d %-15d %-15d\n", 
               processes[i].name, 
               processes[i].arrival_time, 
               processes[i].burst_time, 
               processes[i].priority);
    }
    printf("══════════════════════════════════════════════\n");
    wait_for_step();
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

void print_dashboard(int running_idx, int t, const char* algo_name) {
    printf("\033[H\033[J"); // Clear screen
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

void round_rubin_scheduler() {
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

void processes_cleaning() {
    for (int i = 0; i < process_counter; i++) {
        if (processes[i].pid > 0) {
            terminate_process(processes[i].pid);
            processes[i].pid = -1;
        }
    }
}

int main(int argc, char *argv[]) {
    int opt;
    int quantum = 2; // Default quantum
    char* filename = NULL;

    struct option long_options[] = {
        {"demo", no_argument, &demo_mode, 1},
        {"step", no_argument, &step_mode, 1},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        // Options handled by struct
    }

    if (optind < argc) {
        filename = argv[optind++];
    }
    if (optind < argc) {
        quantum = atoi(argv[optind++]);
    }

    if (!demo_mode && !filename) {
        fprintf(stderr, "Usage: %s [--demo] [--step] [<csv_filename> <quantum>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (quantum <= 0) {
        fprintf(stderr, "Error: Quantum must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    time_quantum = quantum;
    setup_cpu_signal_handlers();

    if (demo_mode) {
        generate_demo_processes();
    } else {
        parse_csv(filename);
    }

    fcfs_scheduler();
    processes_cleaning();
    
    sjf_scheduler();
    processes_cleaning();
    
    priority_scheduler();
    processes_cleaning();
    
    round_rubin_scheduler();
    processes_cleaning();

    return EXIT_SUCCESS;
}
