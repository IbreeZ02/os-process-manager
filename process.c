#include <stdio.h>
#include <stddef.h> //for NULL
#include <stdlib.h>
#include "process.h"

PCB *create_pcb(int id) {
   PCB *p = malloc(sizeof(PCB));
    if (p == NULL) return NULL; //if allocation failed
    p->id = id;
    p->parent_id = -1; //no parent yet
    p->state = NEW;
    p->priority = 0;
    p->start_time = 0;
    p->finish_time = 0;
    p->cpu_time_used = 0;
    p->wakeup_time = -1; //no alarm set yet
    p->arrival_time = 0;
    p->burst_time = 0;
    p->remaining_time = 0;
    p->quantum = 0;
    p->wait_for = -1;
    p->io_remaining = 0;
    p->next = NULL;
    p->mailbox_id = -1; // -1 (no mailbox assigned yet) in case general boxes
    return p;
};

bool destroy_pcb(PCB *p) {
    if (p == NULL) return false;
    free(p);
    return true;
}

void print_pcb(PCB *p) {
    if (p == NULL) {
        printf("[PCB] NULL\n");
        return;
    }

    // state as string
    const char *state_str[] = {"NEW", "READY", "RUNNING", "WAITING", "TERMINATED"};

    printf("-----------------------------\n");
    printf("[PCB] id           : %d\n",   p->id);
    printf("      parent_id    : %d\n",   p->parent_id);
    printf("      state        : %s\n",   state_str[p->state]);
    printf("      priority     : %d\n",   p->priority);
    printf("      start_time   : %d\n",   p->start_time);
    printf("      finish_time  : %d\n",   p->finish_time);
    printf("      cpu_time_used: %d\n",   p->cpu_time_used);
    printf("      wakeup_time  : %d\n",   p->wakeup_time);
    printf("      arrival_time : %d\n",   p->arrival_time);
    printf("      burst_time   : %d\n",   p->burst_time);
    printf("      remaining    : %d\n",   p->remaining_time);
    printf("      quantum      : %d\n",   p->quantum);
    printf("      mailbox_id   : %d\n",   p->mailbox_id);
    printf("      next         : %s\n",   p->next ? "-> next PCB" : "NULL");
    printf("-----------------------------\n");
}