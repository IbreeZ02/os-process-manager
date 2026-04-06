#include <unistd.h>
#include <stdatomic.h>
#include "timer.h"

_Atomic int system_time = 0;
_Atomic int simulation_running = 1;

void* timer_run(void* arg) {
    while (atomic_load(&simulation_running)) {
        usleep(100000);
        atomic_fetch_add(&system_time, 1);
    }
    return NULL;
}