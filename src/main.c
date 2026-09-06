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
#include "include/state.h"
#include "include/process.h"
#include "include/dispatcher.h"
#include "include/scheduler.h"
#include "include/ui.h"

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
    
    round_robin_scheduler();
    processes_cleaning();

    return EXIT_SUCCESS;
}
