//
// Created by worker on 3/8/2026.
//

#ifndef OS_PROCESS_MANAGER_IPC_H
#define OS_PROCESS_MANAGER_IPC_H
#include "mailbox.h"
#include "message_queue.h"
#include "process.h"
#define MAX_PROCESSES 64
//declared here, defined in ipc.c file
extern Mailbox mailbox_table[];
extern MessageQueue *queue;

typedef enum {
    IPC_MAILBOX,    //process to process
    IPC_QUEUE      // Broadcast/ system notifications
} IPC_Type;

void init_ipc(int max_processes);
bool send_ipc(IPC_Type type, int sender_id, int receiver_id, const char* message); //message is to never be modified
Message* recv_ipc(int id);  //Checks both mailbox + system queue
void cleanup_ipc();
//added to facilitate scheduler function (tick) for checking wait queue/wakeup
bool has_message(int id);


/* How it works:
// scheduler signals process 3 to start → mailbox (simple signal)
ipc_send(IPC_MAILBOX, 0, 3, "start");

// process 1 sends data to process 2 → message queue
ipc_send(IPC_QUEUE, 1, 2, "data chunk");

// process 3 checks for ANY message (ipc.h handles priority)
Message *m = ipc_recv(3);
// checks mailbox[3] first → then system_queue for receiver=3
*/


#endif //OS_PROCESS_MANAGER_IPC_H