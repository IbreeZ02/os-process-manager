//
// Created by worker on 2/21/2026.
//

#ifndef OS_PROCESS_MANAGER_SCHEDULER_H
#define OS_PROCESS_MANAGER_SCHEDULER_H
#include "process.h"
#include "process_queues.h"
#include "gantt.h"
#include "interrupt_handler.h"
#define MAX_PROC 64
typedef enum {
    FCFS,
    SJF_NONPREEMPTIVE,
    SRTF, //SJRF (preemptive SJF)
    PRIORITY_PREEMPTIVE,
    PRIORITY_NONPREEMPTIVE,
    ROUNDROBIN,
    //MLQ,
    //MLFQ,
    /*advanced scheduling
    THREAD_SCHEDULING,
    MULTIPROCESSOR
    REALTIME*/
}scheduling_algorithm;

typedef struct {
    int context_switches;
    int total_processes;
    int completed_processes;
    float avg_waiting_time;
    float avg_turnaround_time;
    float cpu_utilization;
} scheduler_stats;

typedef struct scheduler{
    PCB *running;
    process_queue *ready;
    process_queue *wait;
    scheduling_algorithm algo;
    int current_quantum; //for roundrobin & MLFQ
    int default_quantum;
    scheduler_stats stats;
    gantt_log *gantt;
} scheduler;

typedef struct {
    int pid;
    int arrival;
    int burst;
    int priority;
    int finish;
    int turnaround;
    int waiting;
} proc_stat;

extern proc_stat proc_stats[];
extern int proc_count;

scheduler *create_scheduler(scheduling_algorithm algo, int  default_quantum);
void destroy_scheduler(scheduler *s);
//void run_scheduler(scheduler *s); no longer needed: program loop in main
void scheduler_tick(scheduler *s, interrupt_queue *iq); //called every timer tick — check quantum, wakeups - and raise interrupts
PCB *select_next(scheduler *s); //picks next PCB based on algo

//process control
void admit_process(scheduler *s, PCB *p);        //NEW → READY
void preempt_process(scheduler *s);              //RUNNING → READY (forced)
void block_process(scheduler *s, PCB *p);        //RUNNING → WAIT
void unblock_process(scheduler *s, PCB *p);      //WAIT → READY (IO done)
void terminate_process(scheduler *s, PCB *p);    //RUNNING → TERMINATED

void export_stats_json(scheduler *s, const char *filename);
void print_stats_table(scheduler *s);

#endif //OS_PROCESS_MANAGER_SCHEDULER_H
