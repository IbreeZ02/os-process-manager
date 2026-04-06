#ifndef OS_PROCESS_MANAGER_PCB_H
#define OS_PROCESS_MANAGER_PCB_H
#include <stdbool.h>

typedef enum {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} process_state;

//pcb-------------------------------------------------------------------------------------------------------------------
typedef struct process{
    int id;
    int parent_id;
    process_state state;
    int priority;
    int start_time;
    int finish_time;
    int cpu_time_used;
    //time for next alarm: for interrupts
    int wakeup_time;

    //scheduling
    int arrival_time;
    int burst_time;
    int remaining_time;
    int quantum; //roundrobin

    int wait_for; //wait for graph (deadlock)
    int io_remaining; //i/o simulation
    struct process *next; //linked list for queueing

    //IPC
    int mailbox_id;
} PCB;

//functions-------------------------------------------------------------------------------------------------------------
PCB *create_pcb(int id);
bool destroy_pcb(PCB *p); //free space
void print_pcb(PCB *p); //for debugging

#endif //OS_PROCESS_MANAGER_PCB_H