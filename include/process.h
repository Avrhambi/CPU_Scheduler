#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

void parse_csv(const char* filename);
void generate_demo_processes();
void reset_processes();
pid_t create_process();
void terminate_process(pid_t pid);
void processes_cleaning();

#endif
