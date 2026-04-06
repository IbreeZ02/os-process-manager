//
// Created by worker on 2/21/2026.
//

#include "process_queues.h"
#include <stdio.h>
#include <stdlib.h>

process_queue *create_queue(queue_type type) {
    process_queue *q = malloc(sizeof(process_queue));
    if (!q) return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->type = type;
    return q;
};

void enqueue(process_queue *q, PCB *p) {
    //enqueue the process pcb (doesn't create it)
    if(!q || !p) return;
    if (q->tail)
        q->tail->next = p;
    else
        q->head = p;
    q->tail = p; //move tail to last node
    q->size++;
    p->next = NULL;//fine if protected with mutex
};

PCB *dequeue(process_queue *q) {
    if(!q||!q->head) return NULL; //prevent segmentation fault if queue is empty
    PCB *p = q->head; //save head to pop
    q->head = p->next; //set next as new head
    if (q->head == NULL) //empty queue, both head and tail points to the same node
        q->tail = NULL; //dangling pointer
    p->next = NULL; //detach from list
    q->size--;
    return p;
};

bool remove_element(process_queue *q, PCB *p) {
    if(!q||!p||!q->head) return false;

    //if p is head
    if (p == q->head) {
        dequeue(q);
        return true;
    }

    //searching for p (if not head) (starting from first one after head)
    PCB *previous = q->head;
    PCB *current = q->head->next;
    while (current) {
        if (p == current) {
            previous->next = current->next; //removing current
            if (q->tail == current)// if it's last
                q->tail = previous;
            p->next = NULL;
            q->size--;
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;
};

PCB *peek(process_queue *q) {
    if (!q) return NULL;
    return q->head;
};

bool is_empty(process_queue *q) {
    if (!q) return true;
    return q->size==0;
};

void destroy_queue(process_queue *q) {
    if (!q) return;
    while (!is_empty(q)) {
        dequeue(q);
    }
    free(q);
};

int queue_size(process_queue *q) {
    if (!q) return -1; //empty
    return q->size;
};

void print_queue(process_queue *q) {
    if (!q) return;

    const char *type_str[] = { "READY", "WAIT"};
    printf("[%s QUEUE_TYPE: | size=%d] ", type_str[q->type], q->size);

    const char *state_str[] = {"NEW", "READY", "RUNNING", "WAITING", "TERMINATED"};
    PCB *current = q->head;
    while (current) {
        printf("→ PID:%d(%s) ", current->id, state_str[current->state]);
        current = current->next;
    }
    printf("\n");
}

//added
PCB *find_by_pid(process_queue *q, int pid) {
    if (!q) return NULL;
    PCB *current = q->head;
    while (current != NULL) {
        if (current->id == pid)
            return current;
        current = current->next;
    }
    return NULL;
}