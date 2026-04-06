//
// Created by worker on 3/8/2026.
//

#include "mailbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mailbox_init(Mailbox *box){
    if (box == NULL) return;
    box->sender_id = -1;
    box->occupied  = false;
    box->content[0] = '\0';
}

bool mailbox_send(Mailbox* box, int sender_id, const char* message){
    if (!message || ! box) return false;
    if (box->occupied) {
        fprintf(stderr, "[MAILBOX] full — sender %d cannot send\n", sender_id);
        return false;
    }
    strncpy(box->content, message, 256); //message is already a pointer
    box->content[255] = '\0'; //strncpy doesn't guarantee null termination
    box->occupied = true;
    box->sender_id = sender_id;
    return true;
}

Message *mailbox_recv(Mailbox *box) {
    if (box == NULL || !box->occupied) return NULL;
    Message *message = malloc(sizeof(Message));
    if (message == NULL) return NULL;
    message->sender_id = box->sender_id;
    //message->receiver_id = -1; //mailbox doesn't track it
    strncpy(message->content, box->content, 256);
    message->content[255] = '\0';
    message->next = NULL;
    mailbox_clear(box);
    return message;
}

void mailbox_clear(Mailbox *box) {
    if (box == NULL) return;
    box->sender_id  = -1;
    box->occupied   = false;
    box->content[0] = '\0';
    //no need for free() because content is not heap allocated it's a fixed array inside mailbox struct
}

void mailbox_print(Mailbox *box) {
    if (box == NULL) {
        printf("[MAILBOX] NULL\n");
        return;
    }
    printf("-----------------------------\n");
    printf("[MAILBOX] occupied : %s\n",  box->occupied ? "yes" : "no");
    printf("          sender   : %d\n",  box->sender_id);
    printf("          content  : %s\n",  box->occupied ? box->content : "(empty)");
    printf("-----------------------------\n");
}