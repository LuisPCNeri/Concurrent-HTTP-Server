#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <pthread.h>

typedef struct{
    // Array of threads for each worker
    pthread_t* threads;
    int threadNum;
    pthread_mutex_t tMutex;
    pthread_cond_t tCond;
    int shutdown;
} threadPool;

// Creates a thread pool with threadNum number of active threads
// that start with the execution of workerThread ROUTINE
// workerThread MUST take in at least one void* arg argument
threadPool* CreateThreadPool(int threadNum, void* workerThread);

// Frees memory occupied by thread pool struct
void DestroyThreadPool(threadPool* tPool);

#endif