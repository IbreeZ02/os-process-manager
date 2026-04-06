#include "deadlock_WFG.h"
#include <stdio.h>
#include <stdlib.h>

//build
wfg *build_wfg(scheduler *s) {
    if (!s) return NULL;
    wfg *w = malloc(sizeof(wfg));
    if (!w) return NULL;

    int max = queue_size(s->wait);
    w->nodes = malloc(sizeof(wfg_node) * max);
    if (!w->nodes) { free(w); return NULL; }
    w->size = 0;
    // walk wait queue — only include IPC blocked processes
    PCB *current = s->wait->head;
    while (current != NULL) {
        if (current->wait_for != -1) { //only IPC blocked
            w->nodes[w->size].pid = current->id;
            w->nodes[w->size].wait_for = current->wait_for;
            w->size++;
        }
        current = current->next;
    }
    return w;
}


//cycle detection
//helper functions ------------------------------------------------------
static int find_node_wfg(wfg *w, int pid) {
    for (int i = 0; i < w->size; i++)
        if (w->nodes[i].pid == pid) return i;
    return -1;
}

static bool dfs(wfg *w, int idx, bool *visited, bool *in_stack) {
    visited[idx]  = true;
    in_stack[idx] = true;

    int wait_for = w->nodes[idx].wait_for;
    if (wait_for != -1) {
        int next = find_node_wfg(w, wait_for);
        if (next != -1) {
            if (!visited[next] && dfs(w, next, visited, in_stack))
                return true;
            else if (in_stack[next])
                return true;    // back edge = cycle = deadlock
        }
    }

    in_stack[idx] = false;
    return false;
}
//-----------------------------------------------------------------------
bool detect_cycle_wfg(wfg *w) {
    if (!w || w->size == 0) return false;
    bool *visited  = calloc(w->size, sizeof(bool));
    bool *in_stack = calloc(w->size, sizeof(bool));
    if (!visited || !in_stack) {
        free(visited); free(in_stack);
        return false;
    }
    bool cycle = false;
    for (int i = 0; i < w->size; i++) {
        if (!visited[i] && dfs(w, i, visited, in_stack)) {
            cycle = true;
            break;
        }
    }
    free(visited);
    free(in_stack);
    return cycle;
}


//recover
void recover_deadlock(scheduler *s, wfg *w) {
    if (!s || !w || w->size == 0) return;

    PCB *victim = NULL;
    for (int i = 0; i < w->size; i++) {
        PCB *p = find_by_pid(s->wait, w->nodes[i].pid);
        if (!p) continue;
        if (!victim || p->priority > victim->priority)
            victim = p;     // higher number = lower priority = kill first
    }
    if (victim) {
        printf("[DEADLOCK] killing PID %d (priority=%d) to recover\n", victim->id, victim->priority);

        // First, unblock any process waiting for the victim
        PCB *curr = s->wait->head;
        while (curr) {
            PCB *next = curr->next;
            if (curr->wait_for == victim->id) {
                printf("[DEADLOCK] also unblocking PID %d waiting for victim\n", curr->id);
                curr->wait_for = -1;
                unblock_process(s, curr);   // moves to ready queue
            }
            curr = next;
        }

        // Now remove victim from wait queue and terminate
        remove_element(s->wait, victim);
        victim->wait_for = -1;
        terminate_process(s, victim);

        // Pass 2: transitively unblock any process whose wait_for target no longer exists.
        bool found = true;
        while (found) {
            found = false;
            curr = s->wait->head;
            while (curr) {
                PCB *next = curr->next;
                if (curr->wait_for != -1) {
                    bool target_alive =
                        find_by_pid(s->wait, curr->wait_for) != NULL || (s->running && s->running->id == curr->wait_for);
                    if (!target_alive) {
                        curr->wait_for = -1;
                        unblock_process(s, curr);
                        found = true;
                        break;
                    }
                }
                curr = next;
            }
        }
    }
}

// ── print ─────────────────────────────────────────────────────────────────────

void print_wfg(wfg *w) {
    if (!w) return;
    printf("=============================\n");
    printf("[WFG] %d nodes\n", w->size);
    for (int i = 0; i < w->size; i++) {
        printf("  PID:%d → waiting for PID:%d\n",
               w->nodes[i].pid,
               w->nodes[i].wait_for);
    }
    printf("=============================\n");
}

// ── export ────────────────────────────────────────────────────────────────────

void export_wfg_json(wfg *w, char *filename, bool cycle_detected) {
    if (!w || !filename) return;
    FILE *f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "{\n");
    fprintf(f, "  \"deadlock\": %s,\n", cycle_detected ? "true" : "false");
    fprintf(f, "  \"nodes\": [\n");
    for (int i = 0; i < w->size; i++) {
        fprintf(f, "    { \"pid\": %d, \"wait_for\": %d }%s\n",
                w->nodes[i].pid,
                w->nodes[i].wait_for,
                i < w->size - 1 ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
    printf("[WFG] exported to %s\n", filename);
}

// ── destroy ───────────────────────────────────────────────────────────────────

void destroy_wfg(wfg *w) {
    if (!w) return;
    free(w->nodes);
    free(w);
}