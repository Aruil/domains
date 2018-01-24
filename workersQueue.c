#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "workersQueue.h"

wQueue *create_queue(){
    wQueue *newQueue;

    newQueue = malloc(sizeof(wQueue));
    if(newQueue == NULL){
        perror("wQueue malloc");
    }else{
        newQueue->size = 0;
        newQueue->start = NULL;
        newQueue->end = NULL;
    }

    return newQueue;
}

int add_newWorker(wQueue *queue, pid_t pid, int pipeID){
    worker *newWorker;

    newWorker = malloc(sizeof(worker));
    if(newWorker == NULL){
        perror("add_newWorker malloc");
        return -1;
    }else{
        newWorker->pid = pid;
        newWorker->pipeID = pipeID;
        newWorker->next = NULL;
        if(queue->start == NULL){
            queue->start = newWorker;
        }else{
            queue->end->next = newWorker;
        }
        queue->end = newWorker;
        queue->size++;
    }

    return 0;
}

void add_existWorker(wQueue *queue, worker *wrk){
    wrk->next = NULL;
    if(queue->start == NULL){
        queue->start = wrk;
    }else{
        queue->end->next = wrk;
    }
    queue->end = wrk;
    queue->size++;
}

worker *remove_worker(wQueue *queue, pid_t pid){
    worker *cur = queue->start, *toReturn = NULL;

    if(cur->pid == pid){
        queue->start = queue->start->next;
        if(queue->start == NULL) queue->end = NULL;
        toReturn = cur;
    }else{
        while(cur->next != NULL){
            if(cur->next->pid == pid){
                toReturn = cur->next;
                if(cur->next->next == NULL) queue->end = cur;
                cur->next = cur->next->next;
                break;
            }
            cur = cur->next;
        }
    }
    if(toReturn != NULL) queue->size--;

    return toReturn;
}

void destroy_queue(wQueue *queue){
    worker *cur = queue->start, *toDelete;

    while(cur != NULL){
        toDelete = cur;
        cur = cur->next;
        close(toDelete->pipeID);
        free(toDelete);
    }

    free(queue);
}
