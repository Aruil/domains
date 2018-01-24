#ifndef _WORKERSQUEUE_H_
#define _WORKERSQUEUE_H_

typedef struct wQueue wQueue;
typedef struct worker worker;

struct wQueue{
    int     size;
    worker  *start;
    worker  *end;
};

struct worker{
    pid_t   pid;
    int     pipeID;
    worker  *next;
};

wQueue *create_queue();

int add_newWorker(wQueue *queue, pid_t pid, int pipeID);

void add_existWorker(wQueue *queue, worker *wrk);

worker *remove_worker(wQueue *queue, pid_t pid);

void destroy_queue(wQueue *queue);

#endif
