#include "interrupt_handler.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

interrupt_queue *create_interrupt_queue() {
    interrupt_queue *iq = malloc(sizeof(interrupt_queue));
    if (!iq) return NULL;
    iq->entries  = malloc(sizeof(interrupt) * 64); //initial capcaity
    if (!iq->entries) { free(iq); return NULL; }
    iq->size     = 0;
    iq->capacity = 64;
    return iq;
}

void destroy_interrupt_queue(interrupt_queue *iq) {
    if (!iq) return;
    free(iq->entries);
    free(iq);
}

void raise_interrupt(interrupt_queue *iq, interrupt_type type, int pid) {
    if (!iq) return;
    // grow if full
    if (iq->size == iq->capacity) {
        iq->capacity *= 2;
        //so it won't leak on failuer
        interrupt *tmp = realloc(iq->entries, sizeof(interrupt) * iq->capacity);
        if (!tmp) { iq->capacity /= 2; return; }
        iq->entries = tmp;
    }

    iq->entries[iq->size].type = type;
    iq->entries[iq->size].pid  = pid;
    iq->size++;
}

interrupt *next_interrupt(interrupt_queue *iq) {
    if (!iq || iq->size == 0) return NULL;

    // copy first entry
    interrupt *i = malloc(sizeof(interrupt));
    //memory leak if malloc failed
    if (!i) return NULL;
    *i = iq->entries[0];

    // shift queue left
    for (int j = 0; j < iq->size - 1; j++)
        iq->entries[j] = iq->entries[j + 1];
    iq->size--;

    return i;   // caller must free
}


void interrupt_handler(interrupt *i, scheduler *s) {
    if (!i || !s) return;
    switch (i->type) {
        case INT_TIMER:
            // quantum expired → preempt running process
            if (!s->running) {
                fprintf(stderr, "[INT] TIMER fired but no running process\n");
                break;
            }
            printf("\n[INT] !! TIMER INTERRUPT !! → PID %d quantum expired\n", s->running->id);
            usleep(200000);
            printf("[INT] TIMER → preempting PID %d\n", s->running->id);
            preempt_process(s);
            break;

        case INT_IO_COMPLETE: {
            printf("\n[INT] !! IO_COMPLETE INTERRUPT !! → PID %d IO finished\n", i->pid);
            usleep(200000);
            PCB *p = find_by_pid(s->wait, i->pid);  //find it
            if (p) {
                printf("[INT] IO_COMPLETE → unblocking PID %d\n", i->pid);
                unblock_process(s, p);
            }
            else
                fprintf(stderr, "[INT] IO_COMPLETE: PID %d not in wait queue\n", i->pid);
            break;
        }
        default:
            fprintf(stderr, "[INT] unknown interrupt type %d\n", i->type);
    }
    free(i);
}