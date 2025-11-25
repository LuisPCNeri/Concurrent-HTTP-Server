#include <stdlib.h>
#include "threadPool.h"

threadPool* CreateThreadPool(int threadNum, void* workerThread){
    // Allocs space for thread pool
    threadPool* pool = malloc(sizeof(threadPool));

    pool->threads = malloc(sizeof(pthread_t)*threadNum);
    pool->threadNum = threadNum;
    // Shutdown signal set to false
    pool->shutdown = 0;

    pthread_mutex_init(&pool->tMutex, NULL);
    pthread_cond_init(&pool->tCond, NULL);

    // Loop to create all the threads to populate the array
    for(unsigned int i = 0; i < threadNum; i++) pthread_create(&pool->threads[i], NULL, workerThread, &pool);

    return pool;
}

void DestroyThreadPool(threadPool* pool){
    // Join all threads in the thread pool
    for(unsigned int i = 0; i < pool->threadNum; i++){
        pthread_join(pool->threads[i], NULL);
    }

    // Clean up all thread related stuff on thread pool
    free(pool->threads);
    free(&pool->tMutex);
    free(&pool->tCond);

    // Free the actual thread pool allocated memory
    free(&pool);
}