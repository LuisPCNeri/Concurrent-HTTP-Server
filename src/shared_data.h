#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

#define MAX_QUEUE_SIZE 100

#include "semaphores.h"
#include "serverCache.h"

typedef struct{
    long totalRequests;
    long bytesTransferred;
    long status200;
    long status404;
    long status5xx;
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
    semaphore* sem;
    cache* cache;
    int sv[2];
} data;

// Creates a mapped region with data using sv[2] as a socket pair to communicate between processes
data* createSharedData();

// Gets the data with NAME name
data* getSharedData(char* name);
void destroySharedData(data*);

// Adds client socket file descriptor to queue in shared data
void sockEnqueue(connectionQueue* q, int clientFd);
// Takes first client socket file descriptor from queue
int sockDequeue(connectionQueue* q);

// Returns 1 if queue is empty and 0 if not
int IsQueueEmpty(connectionQueue* q);

#endif
