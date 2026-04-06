#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "scheduler.h"
#include "interrupt_handler.h"
#include "timer.h"
#include "ipc.h"
#include "deadlock_WFG.h"
#include "syscall_handler.h"

#define DEADLOCK_CHECK_INTERVAL 10

int syscall_count = 0;
int ipc_count = 0;

// Create a process with random attributes
PCB* create_random_process(int pid) {
    PCB *p = create_pcb(pid);
    if (!p) return NULL;
    p->arrival_time = 0;                // all arrive at time 0
    p->burst_time = rand() % 20 + 1;    // 1..20 ticks
    p->remaining_time = p->burst_time;
    p->priority = rand() % 10;           // 0..9 (lower = higher)
    return p;
}


int main() {
    srand(time(NULL));

    int algo_choice, num_proc, quantum = 0, enable_random;

    printf("=== Process Scheduler Simulator ===\n");
    printf("Select scheduling algorithm:\n");
    printf("0: FCFS\n1: SJF\n2: SRTF\n3: Priority (Preemptive)\n4: Priority (Non-preemptive)\n5: Round Robin\n");
    printf("Enter choice (0-5): ");
    scanf("%d", &algo_choice);
    if (algo_choice < 0 || algo_choice > 5) {
        fprintf(stderr, "Invalid choice.\n");
        return 1;
    }
    scheduling_algorithm algo = (scheduling_algorithm)algo_choice;

    if (algo == ROUNDROBIN) {
        printf("Enter quantum (ticks): ");
        scanf("%d", &quantum);
        if (quantum <= 0) quantum = 1;
    }

    printf("Enter number of processes (1-%d): ", MAX_PROC);
    scanf("%d", &num_proc);
    if (num_proc < 1 || num_proc > MAX_PROC) num_proc = MAX_PROC;

    printf("Enable random syscalls/IPC? (0=no,1=yes): ");
    scanf("%d", &enable_random);

    // Start timer thread
    pthread_t timer_thread;
    simulation_running = 1;
    pthread_create(&timer_thread, NULL, timer_run, NULL);

    scheduler *s = create_scheduler(algo, quantum);
    interrupt_queue *iq = create_interrupt_queue();
    init_ipc(MAX_PROCESSES);

    // Create processes
    for (int i = 0; i < num_proc; i++) {
        PCB *p = create_random_process(i);
        admit_process(s, p);
    }

    int prev_time = 0;

    while (simulation_running && s->stats.completed_processes < num_proc) {
        if (system_time > prev_time) {
            prev_time = system_time;

            scheduler_tick(s, iq);

            interrupt *intr;
            while ((intr = next_interrupt(iq)) != NULL) {
                interrupt_handler(intr, s);
            }

            if (!s->running && !is_empty(s->ready)) {
                PCB *next = select_next(s);
                if (next) {
                    s->running = next;
                    s->running->state = RUNNING;
                    s->running->start_time = system_time;
                    log_gantt(s->gantt, next->id, system_time, system_time, "start");
                    if (s->algo == ROUNDROBIN) s->current_quantum = s->default_quantum;
                }
            }

            // Random syscalls (if enabled)
            if (enable_random && s->running && (rand() % 100 < 10)) { // 10% chance per tick
                syscall_type type = rand() % 4;  // 0:IO,1:EXIT,2:SLEEP,3:RECV
                syscall_request req;
                req.type = type;
                req.pid = s->running->id;

                if (type == SYS_RECV) {
                    // Determine target sender
                    if (rand() % 2 == 0 && num_proc > 1) {
                        // Wait for a specific process (maybe next one)
                        req.target_sender = (s->running->id + 1) % num_proc;
                    } else {
                        req.target_sender = -1;   // any sender
                    }
                    // With 10% probability, make it indefinite; otherwise timeout
                    if (rand() % 100 < 10) {
                        req.duration = 0;          // indefinite block (can create deadlock)
                    } else {
                        req.duration = rand() % 10 + 1; // timeout 1-10 ticks
                    }
                } else {
                    req.duration = rand() % 5 + 1;   // 1-5 ticks for I/O or sleep
                    req.target_sender = -1;
                }

                syscall_count++;
                syscall_handler(&req, s);
                // Note: handler may block or terminate the process
            }

            // Random IPC (if enabled)
            if (enable_random && (rand() % 100 < 5)) { // 5% chance per tick
                int sender = rand() % num_proc;
                int receiver = rand() % num_proc;
                if (sender != receiver) {
                    char msg[64];
                    sprintf(msg, "random msg #%d", rand() % 1000);
                    send_ipc(IPC_QUEUE, sender, receiver, msg);
                    ipc_count++;
                    printf("[MAIN] IPC sent %d→%d\n", sender, receiver);
                }
            }

            // Deadlock detection
            if (system_time % DEADLOCK_CHECK_INTERVAL == 0 && !is_empty(s->wait)) {
                wfg *w = build_wfg(s);
                if (detect_cycle_wfg(w)) {
                    printf("\n!!! DEADLOCK DETECTED at tick %d !!!\n", system_time);
                    export_wfg_json(w, "wfg_before_recovery.json", true);
                    printf("[DEADLOCK] Performing recovery...\n");
                    recover_deadlock(s, w);
                }
                destroy_wfg(w);
            }
        } else {
            usleep(1000);
        }
    }

    simulation_running = 0;
    pthread_join(timer_thread, NULL);

    export_gantt_json(s->gantt, "gantt.json");
    export_stats_json(s, "stats.json");
    print_stats_table(s);

    destroy_scheduler(s);
    destroy_interrupt_queue(iq);
    cleanup_ipc();

    return 0;
}