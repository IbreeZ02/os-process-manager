//
// Created by worker on 3/10/2026.
//

#ifndef OS_PROCESS_MANAGER_SYSCALL_HANDLER_H
#define OS_PROCESS_MANAGER_SYSCALL_HANDLER_H

#include "scheduler.h"

typedef enum {
    SYS_IO, //requesting IO == block
    SYS_EXIT, //process finished == terminate
    SYS_SLEEP,
    SYS_RECV //needed for wait-for msg situation
} syscall_type;

typedef struct {
    syscall_type type;
    int pid;
    int duration; //no of ticks for SYS_SLEEP & IO (until complete), and SYS_RECV
    int target_sender; //for SYS_RECV, for realistic deadlock detection
} syscall_request;

void syscall_handler(syscall_request *req, scheduler *s);


#endif //OS_PROCESS_MANAGER_SYSCALL_HANDLER_H