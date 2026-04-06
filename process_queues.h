#ifndef OS_PROCESS_MANAGER_QUEUES_H
#define OS_PROCESS_MANAGER_QUEUES_H
#include "process.h"

typedef enum {
    READY_Q,
    WAIT_Q
}queue_type;

typedef struct {
    PCB *head;
    PCB *tail;
    int size;
    queue_type type;
}process_queue;

//functions-------------------------------------------------------------------------------------------------------------
process_queue *create_queue(queue_type type);
void destroy_queue(process_queue *q);
void enqueue(process_queue *q, PCB *p);
PCB *dequeue(process_queue *q); //same as pop: removes and return head
bool remove_element(process_queue *q, PCB *p); //remove specific pcb (wait->ready)
PCB *peek(process_queue *q);
bool is_empty(process_queue *q);
int queue_size(process_queue *q);
void print_queue(process_queue *q);
PCB *find_by_pid(process_queue *q, int pid);  //for interrupt IO_complete to find process in wait queue

#endif //OS_PROCESS_MANAGER_QUEUES_H