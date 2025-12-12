#include <stdlib.h>
#include "threadPool.h"
#include "worker.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125264
        Guilherme Mendes Martins NMEC 125260
*/

threadPool* CreateThreadPool(int threadNum, semaphore* sems){
    // Allocs space for thread pool
    threadPool* pool = malloc(sizeof(threadPool));

    pool->threads = malloc(sizeof(pthread_t) * threadNum);
    pool->threadNum = threadNum;
    // Shutdown signal set to false
    pool->shutdown = 0;

    pool->sems = sems;

    pthread_mutex_init(&pool->tMutex, NULL);
    pthread_cond_init(&pool->tCond, NULL);

    // Loop to create all the threads to populate the array
    for(int i = 0; i < threadNum; i++) pthread_create(&pool->threads[i], NULL, workerThread, pool);

    return pool;
}

void DestroyThreadPool(threadPool* pool){
    // Join all threads in the thread pool
    for(int i = 0; i < pool->threadNum; i++){
        pthread_join(pool->threads[i], NULL);
    }

    // Clean up all thread related stuff on thread pool
    pthread_cond_destroy(&pool->tCond);
    pthread_mutex_destroy(&pool->tMutex);

    free(pool->threads);

    // Free the actual thread pool allocated memory
    free(pool);
}