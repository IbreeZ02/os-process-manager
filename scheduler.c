#include "scheduler.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include "ipc.h"
#include <unistd.h>
#include "process.h"

proc_stat proc_stats[MAX_PROC];
int proc_count = 0;


/* The static keyword is important, it keeps algorithm functions private to scheduler.c,
 * only select_next is exposed. Callers never need to know which algorithm is running,
 * they just call select_next. */
//PROTOTYPES:
static PCB *select_FCFS(process_queue *ready);
static PCB *select_SJF(process_queue *ready);
static PCB *select_SRTF(process_queue *ready, PCB *running);
static PCB *select_priority(process_queue *ready);
static PCB *select_RR(scheduler *s);
//static PCB *select_MLQ(scheduler *s);
//static PCB *select_MLFQ(scheduler *s);


scheduler *create_scheduler(scheduling_algorithm algo, int default_quantum) {
    scheduler *s = malloc(sizeof(scheduler));
    if (!s) return NULL;
    s->ready = create_queue(READY_Q);
    s->wait = create_queue(WAIT_Q);
    if (!s->ready || !s->wait) {
        free(s);
        return NULL;
    }
    s->running = NULL;
    s->algo = algo;
    s->default_quantum = default_quantum;
    s->current_quantum = default_quantum;
    s->gantt = create_gantt();
    s->stats.context_switches = 0;
    s->stats.total_processes = 0;
    s->stats.completed_processes = 0;
    s->stats.avg_waiting_time = 0.0f;
    s->stats.avg_turnaround_time = 0.0f;
    s->stats.cpu_utilization = 0.0f;
    return s;
}

void destroy_scheduler(scheduler *s) {
    if (!s) return;
    destroy_queue(s->ready);
    destroy_queue(s->wait);
    destroy_gantt(s->gantt);
    free(s);
}

//called every timer tick — check quantum, performs wake-ups
void scheduler_tick(scheduler *s, interrupt_queue *iq) {
    if (!s || !iq) return;
    //decrement times (&quantum) for running processes - and terminate finished ones
    if (s->running) {
        s->running->remaining_time--;
        s->running->cpu_time_used++;
        if (s->algo == ROUNDROBIN /*|| s->algo == MLQ || s->algo == MLFQ*/) {
            s->current_quantum--;
        }
        if (s->running->remaining_time <= 0) {
            printf("\n[TICK] PID %d finished execution\n", s->running->id);
            usleep(200000);
            terminate_process(s, s->running);
            return;
        }
        //if quantum expired no interrupt queue needed
        if ((s->algo == ROUNDROBIN /*|| s->algo == MLQ || s->algo == MLFQ*/) && s->current_quantum <= 0) {
            printf("\n[TICK] PID %d quantum expired -> raising INT_TIMER\n", s->running->id);
            raise_interrupt(iq, INT_TIMER, s->running->id); //raise interrupt
            //usleep(200000);
            //preempt_process(s);
        }
    }

    //checking wait queue/ waking up processes
    PCB *current = s->wait->head;
    while (current != NULL) {
        PCB *next = current->next;

        //1. IO_COMPLETE interrupt
        if (current->io_remaining > 0) {
            current->io_remaining--;
            if (current->io_remaining == 0) {
                printf("\n[TICK] PID %d IO complete → raising INT_IO_COMPLETE\n", current->id);
                raise_interrupt(iq, INT_IO_COMPLETE, current->id);
                //usleep(200000);
                //unblock_process(s, current);
            }
        }
        //2. wake up for SYS_SLEEP, no interrupt -> handled directly
        else if (current->wakeup_time != -1 && system_time >= current->wakeup_time) {
            printf("\n[TICK] PID %d wakeup time reached → unblocking\n", current->id);
            usleep(200000);
            current->wakeup_time = -1;
            current->wait_for = -1; //so it won't stuck waiting
            unblock_process(s, current); //WAIT → READY
        }
        //3. check for IPC, no interrupt
        else {
            if (has_message(current->id)) {
                printf("\n[TICK] PID %d message arrived → unblocking\n", current->id);
                usleep(200000);
                unblock_process(s, current);
            }
        }
        current = next;
    }
}

//picks next PCB based on algo
PCB *select_next(scheduler *s) {
    switch (s->algo) {
        case FCFS: return select_FCFS(s->ready);
        case SJF_NONPREEMPTIVE: return select_SJF(s->ready);
        case SRTF: return select_SRTF(s->ready, s->running);
        case PRIORITY_PREEMPTIVE: //fall-through same return as below
        case PRIORITY_NONPREEMPTIVE: return select_priority(s->ready);
        case ROUNDROBIN: return select_RR(s);
        //case MLQ: return select_MLQ(s);
        //case MLFQ: return select_MLFQ(s);
        default: return select_FCFS(s->ready);
    }
}

void print_stats(scheduler *s) {
    if (!s) return;
    const char *algo_str[] = {"FCFS", "SJF", "SRTF", "PRIORITY_P", "PRIORITY_NP", "RR", "MLQ", "MLFQ"};
    printf("[STATS]=============================\n");
    printf("algorithm       : %s\n", algo_str[s->algo]);
    printf("total processes : %d\n", s->stats.total_processes);
    printf("context switches: %d\n", s->stats.context_switches);
    printf("avg waiting     : %.2f ticks\n", s->stats.avg_waiting_time);
    printf("avg turnaround  : %.2f ticks\n", s->stats.avg_turnaround_time);
    printf("[STATS]=============================\n");
}


//process control---------------------------------------------------------------------------------
void admit_process(scheduler *s, PCB *p) {
    if (!s || !p) return;
    p->state = READY;
    //p->arrival_time = system_time; made random in main
    enqueue(s->ready, p);
    s->stats.total_processes++;
    printf("[SCHEDULER] PID %d admitted → READY\n", p->id);
}

void preempt_process(scheduler *s) {
    if (!s || !s->running) return;
    PCB *p = s->running;
    log_gantt(s->gantt, p->id, p->start_time,system_time, "preempted");
    p->state = READY;
    s->running = NULL;
    enqueue(s->ready, p);
    s->stats.context_switches++;
    s->current_quantum = s->default_quantum;
    printf("[SCHEDULER] PID %d preempted → READY\n", p->id);
}

void block_process(scheduler *s, PCB *p) {
    if (!s || !p) return;
    log_gantt(s->gantt, p->id, p->start_time, system_time, "blocked");
    p->state = WAITING;
    if (s->running==p)s->running = NULL; //only cleared it's running
    enqueue(s->wait, p);
    s->stats.context_switches++;
    printf("[SCHEDULER] PID %d blocked → WAIT\n", p->id);
}

void unblock_process(scheduler *s, PCB *p) {
    if (!s || !p) return;
    remove_element(s->wait, p);
    p->state = READY;
    enqueue(s->ready, p);
    printf("[SCHEDULER] PID %d unblocked → READY\n", p->id);
}

void terminate_process(scheduler *s, PCB *p) {
    if (!s || !p) return;
    p->state = TERMINATED;
    p->finish_time = system_time;
    if (s->running == p) s->running = NULL;
    //s->running = NULL; incorrectly clears the running pointer, losing the currently executing process.
    log_gantt(s->gantt, p->id, p->start_time, p->finish_time, "terminated");
    s->stats.context_switches++;
    s->stats.completed_processes++;
    int turnaround = p->finish_time - p->arrival_time;
    int waiting = turnaround - p->burst_time;
    //incremental averaging
    s->stats.avg_turnaround_time += ((float)turnaround - s->stats.avg_turnaround_time) / s->stats.completed_processes;
    s->stats.avg_waiting_time += ((float)waiting - s->stats.avg_waiting_time) / s->stats.completed_processes;
    printf("[SCHEDULER] PID %d terminated (turnaround=%d, wait=%d)\n", p->id, turnaround, waiting);
    // Inside terminate_process, after calculating turnaround and waiting:
    if (proc_count < MAX_PROC) {
        proc_stats[proc_count].pid = p->id;
        proc_stats[proc_count].arrival = p->arrival_time;
        proc_stats[proc_count].burst = p->burst_time;
        proc_stats[proc_count].priority = p->priority;
        proc_stats[proc_count].finish = p->finish_time;
        proc_stats[proc_count].turnaround = turnaround;
        proc_stats[proc_count].waiting = waiting;
        proc_count++;
    }
    destroy_pcb(p);
}


//algorithms -------------------------------------------------------------------------------------
static PCB *select_FCFS(process_queue *ready) {
    if (!ready || is_empty(ready)) return NULL;
    return dequeue(ready);
}

static PCB *select_SJF(process_queue *ready) {
    if (!ready || is_empty(ready)) return NULL;
    PCB *shortest = ready->head;
    PCB *current  = ready->head->next;
    while (current != NULL) {
        if (current->burst_time < shortest->burst_time)
            shortest = current;
        current = current->next;
    }
    remove_element(ready, shortest);
    return shortest;
}

static PCB *select_SRTF(process_queue *ready, PCB *running) {
    if (!ready || is_empty(ready)) return NULL;
    PCB *shortest = ready->head;
    PCB *current  = ready->head->next;
    while (current != NULL) {
        if (current->remaining_time < shortest->remaining_time)
            shortest = current;
        current = current->next;
    }
    //preempting only if there's a shorter job in queue, else nothing happen,running process keeps CPU
    if (running && running->remaining_time < shortest->remaining_time) //with = it cause an additional unecessary context switch
        return NULL;
    //preempting
    remove_element(ready, shortest);
    return shortest;
}

static PCB *select_priority(process_queue *ready) {
    if (!ready || is_empty(ready)) return NULL;
    //lowest number = highest priority
    PCB *highest = ready->head;
    PCB *current = ready->head->next;
    while (current != NULL) {
        if (current->priority < highest->priority)
            highest = current;
        current = current->next;
    }
    remove_element(ready, highest);
    return highest;
}

static PCB *select_RR(scheduler *s) {
    //quantum expiration preemption handled in tick function
    return dequeue(s->ready);
}

/*
static PCB *select_MLQ(scheduler *s) {
    // placeholder — needs multiple ready queues by priority level
    // for now falls back to priority selection
    return select_priority(s->ready);
}

static PCB *select_MLFQ(scheduler *s) {
    // placeholder — needs multiple queues with different quantums per level
    // for now falls back to RR
    return select_RR(s);
}
*/

// Export statistics to JSON
void export_stats_json(scheduler *s, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", filename);
        return;
    }

    const char *algo_str[] = {"FCFS", "SJF", "SRTF", "PRIORITY_P", "PRIORITY_NP", "RR"};
    fprintf(f, "{\n");
    fprintf(f, "  \"algorithm\": \"%s\",\n", algo_str[s->algo]);
    fprintf(f, "  \"quantum\": %d,\n", s->default_quantum);
    fprintf(f, "  \"total_processes\": %d,\n", s->stats.total_processes);
    fprintf(f, "  \"completed_processes\": %d,\n", s->stats.completed_processes);
    fprintf(f, "  \"context_switches\": %d,\n", s->stats.context_switches);
    fprintf(f, "  \"avg_waiting_time\": %.2f,\n", s->stats.avg_waiting_time);
    fprintf(f, "  \"avg_turnaround_time\": %.2f,\n", s->stats.avg_turnaround_time);
    fprintf(f, "  \"syscall_count\": %d,\n", syscall_count);
    fprintf(f, "  \"ipc_count\": %d,\n", ipc_count);
    fprintf(f, "  \"processes\": [\n");
    for (int i = 0; i < proc_count; i++) {
        fprintf(f, "    { \"pid\": %d, \"arrival\": %d, \"burst\": %d, \"priority\": %d, "
                   "\"finish\": %d, \"turnaround\": %d, \"waiting\": %d }%s\n",
                proc_stats[i].pid, proc_stats[i].arrival, proc_stats[i].burst,
                proc_stats[i].priority, proc_stats[i].finish,
                proc_stats[i].turnaround, proc_stats[i].waiting,
                (i < proc_count - 1) ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
    printf("[STATS] exported to %s\n", filename);
}

// Print statistics table
void print_stats_table(scheduler *s) {
    const char *algo_str[] = {"FCFS", "SJF", "SRTF", "PRIORITY_P", "PRIORITY_NP", "RR"};
    printf("\n========== SIMULATION STATISTICS ==========\n");
    printf("Algorithm            : %s\n", algo_str[s->algo]);
    if (s->algo == ROUNDROBIN) printf("Quantum              : %d\n", s->default_quantum);
    printf("Total processes      : %d\n", s->stats.total_processes);
    printf("Completed processes  : %d\n", s->stats.completed_processes);
    printf("Context switches     : %d\n", s->stats.context_switches);
    printf("Avg waiting time     : %.2f ticks\n", s->stats.avg_waiting_time);
    printf("Avg turnaround time  : %.2f ticks\n", s->stats.avg_turnaround_time);
    printf("Syscalls issued      : %d\n", syscall_count);
    printf("IPC messages sent    : %d\n", ipc_count);
    printf("============================================\n\n");

    printf("Per-process details:\n");
    printf("PID\tArrival\tBurst\tPriority\tFinish\tTurnaround\tWaiting\n");
    for (int i = 0; i < proc_count; i++) {
        printf("%d\t%d\t%d\t%d\t\t%d\t%d\t\t%d\n",
               proc_stats[i].pid, proc_stats[i].arrival, proc_stats[i].burst,
               proc_stats[i].priority, proc_stats[i].finish,
               proc_stats[i].turnaround, proc_stats[i].waiting);
    }
}