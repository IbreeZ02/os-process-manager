//
// Created by worker on 3/8/2026.
//
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>

Mailbox mailbox_table[MAX_PROCESSES];
MessageQueue *queue = NULL;
static int ipc_max_processes = 0;

//initialize both mailbox & queue
void init_ipc(int max_processes){
    ipc_max_processes = max_processes;
    for (int i=0; i<max_processes; i++){
        //empty mailboxes
        mailbox_init(&mailbox_table[i]);
    }
    queue = create_message();
}

//pick mailbox or message queue
bool send_ipc(IPC_Type type, int sender_id, int receiver_id, const char* message){
    if (type == IPC_MAILBOX){
        //guard against invalid receiver
        if (receiver_id < 0 || receiver_id >= ipc_max_processes) {
            fprintf(stderr, "[IPC] invalid receiver_id %d\n", receiver_id);
            return false;
        }
        return mailbox_send(&mailbox_table[receiver_id], sender_id, message);
    }
    else{
        if (queue == NULL) {
            fprintf(stderr, "[IPC] system_queue not initialized\n");
            return false;
        }
        return send_message(queue, sender_id, receiver_id, message);
    }
}

Message* recv_ipc(int id) {   // checks mailbox first, then system queue
    if (id < 0 || id >= ipc_max_processes) {
        fprintf(stderr, "[IPC] invalid id %d\n", id);
        return NULL;
    }
    // mailbox
    Message* msg = mailbox_recv(&mailbox_table[id]);
    if (msg != NULL)
        return msg;

    // message queue
    if (queue != NULL) {
        msg = recv_message(queue, id);
        if (msg != NULL)
            return msg;
    }
    return NULL;
}

void cleanup_ipc(){
    for (int i=0; i< ipc_max_processes; i++){
        mailbox_clear(&mailbox_table[i]);
    }
    if (queue != NULL){
        delete_message(queue);
        queue = NULL;
    }
}

//added
bool has_message(int id) {
    if (id < 0 || id >= ipc_max_processes) return false;
    //checking mailbox
    if (mailbox_table[id].occupied) return true;
    //checking message queue
    if (queue == NULL) return false;
    Message *current = queue->head;
    while (current != NULL) {
        if (current->receiver_id == id) return true;
        current = current->next;
    }
    return false;
}