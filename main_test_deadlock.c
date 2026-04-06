#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "interrupt_handler.h"
#include "timer.h"
#include "ipc.h"
#include "deadlock_WFG.h"
#include "syscall_handler.h"
#include "scheduler.h"

#define DEADLOCK_CHECK_INTERVAL 10
#define NUM_PROC 5

int syscall_count = 0;
int ipc_count = 0;

static int recv_target[NUM_PROC];
static int has_issued_recv[NUM_PROC] = {0};

static void force_recv(scheduler *s, PCB *p, int target) {
    printf("[TEST] Forcing PID %d to issue SYS_RECV for PID %d\n", p->id, target);
    syscall_request req;
    req.type     = SYS_RECV;
    req.pid      = p->id;
    req.duration = 0;           // indefinite block
    req.target_sender = target;
    syscall_count++;
    syscall_handler(&req, s);   // blocks the process, s->running becomes NULL
}

PCB *create_test_process(int pid, int priority) {
    PCB *p = create_pcb(pid);
    if (!p) return NULL;
    p->arrival_time   = 0;
    p->burst_time     = 5;      // small but nonzero; process blocks before consuming it
    p->remaining_time = p->burst_time;
    p->priority       = priority;
    return p;
}


int main() {
    srand(42);

    // Build the cycle: Pi waits for P(i+1) % NUM_PROC
    for (int i = 0; i < NUM_PROC; i++)
        recv_target[i] = (i + 1) % NUM_PROC;

    printf("\n=== DEADLOCK TEST ===\n");
    printf("Circular wait-for chain:\n");
    for (int i = 0; i < NUM_PROC; i++)
        printf("  P%d → P%d\n", i, recv_target[i]);
    printf("\n");

    // Use FCFS: processes run one at a time, no preemption
    simulation_running = 1;
    pthread_t timer_thread;
    pthread_create(&timer_thread, NULL, timer_run, NULL);

    scheduler      *s  = create_scheduler(FCFS, 1);
    interrupt_queue *iq = create_interrupt_queue();
    init_ipc(NUM_PROC);

    for (int i = 0; i < NUM_PROC; i++) {
        PCB *p = create_test_process(i, i);   // priority = i (higher = lower priority)
        admit_process(s, p);
    }

    int prev_time       = 0;
    int deadlock_detected = 0;

    printf("Starting simulation...\n\n");

    while (simulation_running && s->stats.completed_processes < NUM_PROC) {
        if (system_time > prev_time) {
            prev_time = system_time;

            scheduler_tick(s, iq);

            interrupt *intr;
            while ((intr = next_interrupt(iq)) != NULL)
                interrupt_handler(intr, s);

            // Schedule next process if CPU is idle
            if (!s->running && !is_empty(s->ready)) {
                PCB *next = select_next(s);
                if (next) {
                    s->running            = next;
                    s->running->state     = RUNNING;
                    s->running->start_time = system_time;
                    log_gantt(s->gantt, next->id, system_time, system_time, "start");
                    printf("[TICK %d] PID %d started running\n", system_time, next->id);
                }
            }

            // First time a process runs: force it to block on SYS_RECV
            if (s->running) {
                int pid = s->running->id;
                if (!has_issued_recv[pid]) {
                    force_recv(s, s->running, recv_target[pid]);
                    has_issued_recv[pid] = 1;
                    // s->running is now NULL (process is in wait queue)
                }
            }

            // Deadlock detection every DEADLOCK_CHECK_INTERVAL ticks
            if (system_time % DEADLOCK_CHECK_INTERVAL == 0 && !is_empty(s->wait)) {
                wfg *w = build_wfg(s);
                if (detect_cycle_wfg(w)) {
                    if (!deadlock_detected) {
                        printf("\n!!! DEADLOCK DETECTED at tick %d !!!\n", system_time);
                        print_wfg(w);
                        export_wfg_json(w, "wfg_before_recovery.json", true);
                        deadlock_detected = 1;
                    }
                    // recover_deadlock kills one victim AND transitively unblocks
                    // any process whose wait target no longer exists — so a single
                    // call is enough to resolve the entire cycle + leftover chain.
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

    printf("\n=== SIMULATION FINISHED ===\n");
    printf("Total ticks        : %d\n",   system_time);
    printf("Processes completed: %d/%d\n", s->stats.completed_processes, NUM_PROC);
    if (deadlock_detected)
        printf("Deadlock detected and recovered successfully.\n");
    else
        printf("No deadlock occurred (unexpected).\n");

    destroy_scheduler(s);
    destroy_interrupt_queue(iq);
    cleanup_ipc();
    return 0;
}