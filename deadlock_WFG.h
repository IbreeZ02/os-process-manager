//
// Created by worker on 3/14/2026.
//

#ifndef OS_PROCESS_MANAGER_DEADLOCK_WFG_H
#define OS_PROCESS_MANAGER_DEADLOCK_WFG_H

#include "scheduler.h"

typedef struct {
    int pid;
    int wait_for;    // directly from PCB — much simpler than RAG
} wfg_node;

typedef struct {
    wfg_node *nodes;
    int size;
} wfg;

wfg  *build_wfg(scheduler *s);
bool  detect_cycle_wfg(wfg *w);
void  recover_deadlock(scheduler *s, wfg *w);
void  print_wfg(wfg *w);
void  export_wfg_json(wfg *w, char *filename, bool cycle_detected);
void  destroy_wfg(wfg *w);

#endif
