//
// Created by worker on 3/8/2026.
//

#include "message_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

MessageQueue *create_message(){
    MessageQueue *queue = malloc(sizeof(MessageQueue));
    if (queue == NULL) return NULL; //if alloation failed
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

bool send_message(MessageQueue *queue, int sender_id, int receiver_id, const char *content){
    if (queue == NULL || content == NULL) return false;
    Message *message = malloc(sizeof(Message));
    if (message == NULL) return false; //allocation failed
    message->sender_id = sender_id;
    message->receiver_id = receiver_id;
    strncpy(message->content, content, 255);
    message->content[255] = '\0';
    message->next = NULL;
    //enqueue FIFO
    if (queue->tail == NULL){
        queue->head = message; //first message
        queue->tail = message;
    }
    else{
        //link at end then update tail
        queue->tail->next = message;
        queue->tail = message;
    }
    queue->size++;
    return true;
}

Message *recv_message(MessageQueue *queue, int receiver_id){
    if (queue == NULL || queue->head == NULL) return NULL;
    Message *current = queue->head;
    Message *previous = NULL;
    while (current != NULL) {
        if (current->receiver_id == receiver_id) { //looking for the message tha belongs to the receiver
            if (previous == NULL) queue->head = current->next; //message is head -> remove head
            else previous->next  = current->next;  //remove from middle (prev will point to next == current unlinked)
            if (queue->tail == current) queue->tail = previous;
            current->next = NULL;   // clean up before returning
            queue->size--;
            return current; // caller must free
        }
        previous = current;
        current = current->next;
    }
    return NULL;
}

void delete_message(MessageQueue *queue) {
    if (queue == NULL) return;
    Message *current = queue->head;
    while (current != NULL) {
        Message *next = current->next;
        free(current);
        current = next;
    }
    free(queue);
}


//debugging
void show_message(MessageQueue *queue) {
    if (queue == NULL) {
        printf("[MSG_QUEUE] NULL\n");
        return;
    }
    if (queue->size == 0) {
        printf("[MSG_QUEUE] empty\n");
        return;
    }

    printf("-----------------------------\n");
    printf("[MSG_QUEUE] size: %d\n", queue->size);
    Message *curr = queue->head;
    int i = 0;
    while (curr != NULL) {
        printf("  [%d] sender=%d receiver=%d content=\"%s\"\n",
               i++, curr->sender_id, curr->receiver_id, curr->content);
        curr = curr->next;
    }
    printf("-----------------------------\n");
}