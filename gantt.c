//
// Created by worker on 3/10/2026.
//

#include "gantt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

gantt_log *create_gantt() {
    gantt_log *g = malloc(sizeof(gantt_log));
    if (!g) return NULL;
    g->entries = malloc(sizeof(gantt_entry) * 64);
    g->size = 0;
    g->capacity = 64;
    return g;
}

void destroy_gantt(gantt_log *g) {
    if (!g) return;
    free(g->entries);
    free(g);
}

void log_gantt(gantt_log *g, int pid, int start, int end, const char *reason) {
    if (!g) return;

    // grow if full
    if (g->size == g->capacity) {
        g->capacity *= 2;
        //reallocting directly is unsafe, if failes memory is lost
        gantt_entry *tmp = realloc(g->entries, sizeof(gantt_entry)*g->capacity);
        if (!tmp) return;
        g->entries = tmp;
        //if (!g->entries) return; redundant
    }
    g->entries[g->size].pid = pid;
    g->entries[g->size].start_tick = start;
    g->entries[g->size].end_tick = end;
    //g->entries[g->size].reason = reason; for pointers
    strncpy(g->entries[g->size].reason, reason, sizeof(g->entries[g->size].reason) - 1);
    g->entries[g->size].reason[sizeof(g->entries[g->size].reason) - 1] = '\0';
    g->size++;
}

void export_gantt_json(gantt_log *g, const char *filename) {
    if (!g || !filename) return;

    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "[GANTT] could not open %s\n", filename);
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"entries\": [\n");
    for (int i = 0; i < g->size; i++) {
        fprintf(f, "    { \"pid\": %d, \"start\": %d, \"end\": %d, \"reason\": \"%s\" }%s\n",
                g->entries[i].pid,
                g->entries[i].start_tick,
                g->entries[i].end_tick,
                g->entries[i].reason,
                i < g->size - 1 ? "," : ""   // no trailing comma on last entry
        );
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
    printf("[GANTT] exported to %s\n", filename);
}

void print_gantt(gantt_log *g) {
    if (!g) return;
    printf("=============================\n");
    printf("[GANTT] %d entries\n", g->size);
    for (int i = 0; i < g->size; i++) {
        printf("  PID:%d | tick %d → %d | %s\n",
               g->entries[i].pid,
               g->entries[i].start_tick,
               g->entries[i].end_tick,
               g->entries[i].reason);
    }
    printf("=============================\n");
}