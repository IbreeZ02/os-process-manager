#ifndef TIMER_H
#define TIMER_H

extern _Atomic int system_time;
extern _Atomic int simulation_running;
extern int syscall_count;
extern int ipc_count;

void* timer_run(void* arg);

#endif