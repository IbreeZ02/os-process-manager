//
// Created by worker on 3/10/2026.
//

//#ifndef OS_PROCESS_MANAGER_GANTT_H
//#define OS_PROCESS_MANAGER_GANTT_H

#ifndef GANTT_H
#define GANTT_H

typedef struct {
    int pid;
    int start_tick;
    int end_tick;
    char reason[16]; //a pointer might dangle if passed dynamically // "quantum", "blocked", "terminated", "preempted", "ipc_blocked"
} gantt_entry;

typedef struct {
    gantt_entry *entries;
    int size;
    int capacity;
} gantt_log;

gantt_log *create_gantt();
void destroy_gantt(gantt_log *g);
void log_gantt(gantt_log *g, int pid, int start, int end, const char *reason);
void export_gantt_json(gantt_log *g, const char *filename); //web reads this file
void print_gantt(gantt_log *g);

#endif //OS_PROCESS_MANAGER_GANTT_H