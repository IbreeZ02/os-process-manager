//
// Created by worker on 3/10/2026.
//

#ifndef OS_PROCESS_MANAGER_INTERRUPT_HANDLER_H
#define OS_PROCESS_MANAGER_INTERRUPT_HANDLER_H

//#include "scheduler.h" circular include
typedef struct scheduler scheduler; //forward declaration


typedef enum {
    INT_TIMER,       //quantum expired
    INT_IO_COMPLETE, //IO finished → unblock process
} interrupt_type;

typedef struct {
    interrupt_type type;
    int pid;
} interrupt;

// interrupts are asynchronous and need to be handled in order
typedef struct {
    interrupt *entries;
    int size;
    int capacity;
} interrupt_queue;

interrupt_queue *create_interrupt_queue();
void destroy_interrupt_queue(interrupt_queue *iq);
void raise_interrupt(interrupt_queue *iq, interrupt_type type, int pid);
interrupt *next_interrupt(interrupt_queue *iq);
void interrupt_handler(interrupt *i, scheduler *s);


#endif // OS_PROCESS_MANAGER_INTERRUPT_HANDLER_H
