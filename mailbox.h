//
// Created by worker on 3/8/2026.
//

#ifndef OS_PROCESS_MANAGER_MAILBOX_H
#define OS_PROCESS_MANAGER_MAILBOX_H
#include "message_queue.h"   // for Message*
#include <stdbool.h>

typedef struct {
    int sender_id;
    bool occupied;
    char content[256];
} Mailbox;

void mailbox_init(Mailbox *box);
bool mailbox_send(Mailbox* box, int sender_id, const char* message);
Message *mailbox_recv(Mailbox *box);
void mailbox_clear(Mailbox *box);
void mailbox_print(Mailbox *box);

#endif //OS_PROCESS_MANAGER_MAILBOX_H