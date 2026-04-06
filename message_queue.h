//
// Created by worker on 3/8/2026.
//

#ifndef OS_PROCESS_MANAGER_MESSAGE_QUEUE_H
#define OS_PROCESS_MANAGER_MESSAGE_QUEUE_H
#include <stdbool.h>

typedef struct message{
    int sender_id;
    int receiver_id;
    char content[256];
    struct message *next;
}Message;

typedef struct {
    Message *head;
    Message *tail;
    int size;
} MessageQueue;

MessageQueue *create_message(); //pointer so it can be shared (lives on heap)
bool send_message(MessageQueue *message, int sender_id, int receiver_id, const char *content);
Message *recv_message(MessageQueue *message, int receiver_id);
void delete_message(MessageQueue *message);
//bool message_is_empty(MessageQueue *message);
//int message_size(MessageQueue *message);
void show_message(MessageQueue *message); //debugging

#endif //OS_PROCESS_MANAGER_MESSAGE_QUEUE_H