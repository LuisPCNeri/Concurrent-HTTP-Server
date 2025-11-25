#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

#define MAX_QUEUE_SIZE 100

typedef struct{
    long totalRequests;
    long bytesTransferred;
    long status200;
    long status404;
    long status500;
    int activeConnetions;
} serverStats;


typedef struct{
    int socketQueue[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int size;
} connectionQueue;

typedef struct{
    connectionQueue queue;
    serverStats stats;
} data;

data* createSharedData();
void destroySharedData(data*);

// Adds client socket file descriptor to queue in shared data
void sockEnqueue(connectionQueue* q, int clientFd);
// Takes first client socket file descriptor from queue
int sockDequeue(connectionQueue* q);

// Returns 1 if queue is empty and 0 if not
int IsQueueEmpty(connectionQueue* q);

#endif
