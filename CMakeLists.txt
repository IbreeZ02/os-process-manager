cmake_minimum_required(VERSION 4.0)
project(os_process_manager C)

set(CMAKE_C_STANDARD 23)

add_executable(os_process_manager
        process.c
        process.h
        process_queues.c
        process_queues.h
        scheduler.c
        scheduler.h
        timer.c
        timer.h
        message_queue.c
        message_queue.h
        mailbox.c
        mailbox.h
        ipc.h
        ipc.c
        gantt.c
        gantt.h
        interrupt_handler.c
        interrupt_handler.h
        syscall_handler.c
        syscall_handler.h
        deadlock_WFG.c
        deadlock_WFG.h
        interrupt_handler.h
        main.c
)
