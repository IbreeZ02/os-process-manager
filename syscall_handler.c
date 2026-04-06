//
// Created by worker on 3/10/2026.
//

#include "syscall_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include <unistd.h>
#include "message_queue.h"
#include "ipc.h"

void syscall_handler(syscall_request *req, scheduler *s) {
    if (!s || !s->running) return;
    PCB *p = s->running;
    switch(req->type) {
        case SYS_IO:
            printf("[SYSCALL] PID %d → SYS_IO requested (duration=%d ticks)\n", p->id, req->duration);
            usleep(300000); //300ms pause — "CPU stops"
            printf("[SYSCALL] PID %d → SYS_IO granted, blocking...\n", p->id);
            p->io_remaining = req->duration;
            block_process(s, p); //running to wait
            break;
        case SYS_EXIT:
            printf("[SYSCALL] PID %d → SYS_EXIT\n", p->id);
            usleep(300000);
            terminate_process(s, p);
            break;
        case SYS_SLEEP:
            printf("[SYSCALL] PID %d → SYS_SLEEP requested (duration=%d ticks)\n", p->id, req->duration);
            usleep(300000);
            printf("[SYSCALL] PID %d → SYS_SLEEP granted, blocking...\n", p->id);
            p->wakeup_time = system_time + req->duration;
            block_process(s, p);
            break;
            // syscall_handler.c – SYS_RECV case
        case SYS_RECV: {
            printf("[SYSCALL] PID %d → SYS_RECV requested", p->id);
            if (req->target_sender != -1)
                printf(" from PID %d\n", req->target_sender);
            else
                printf(" (any sender)\n");
            usleep(300000);

            // Try to receive a message (no PCB pointer now)
            Message *msg = recv_ipc(p->id);
            if (msg != NULL) {
                printf("[SYSCALL] PID %d → message received from PID %d: %s\n",
                       p->id, msg->sender_id, msg->content);
                p->wait_for = -1;   // not waiting anymore
                free(msg);
            } else {
                printf("[SYSCALL] PID %d → no message, blocking...\n", p->id); //can cause indifinite blocking
                // Record who we are waiting for (maybe -1 for "any")
                if (req->duration > 0) {
                    printf("will wait up to %d ticks\n", req->duration);
                    p->wakeup_time = system_time + req->duration;   // set timeout
                } else {
                    printf("blocking indefinitely\n");
                }
                p->wait_for = req->target_sender;
                block_process(s, p);
            }
            break;
        }
        default:
            fprintf(stderr, "[SYSCALL] unknown type %d\n", req->type);
    }
}