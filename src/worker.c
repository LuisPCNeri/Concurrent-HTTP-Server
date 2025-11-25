#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "shared_data.h"
#include "threadPool.h"
#include "worker.h"
#include "semaphores.h"

void* workerThread(void* arg){
    // Convert argument passed on thread create in threadPool.c to 
    threadPool* pool = (threadPool*) arg;
    
    // Open shared memory segment to use
    int shmFd = shm_open("/webServer_shm", O_RDWR, NULL);
    if(shmFd == -1) return NULL;    // Failed to create shm

    // Try to set shared memory size to size of data
    if (ftruncate(shmFd, sizeof(data)) == -1) {
        close(shmFd);
        return NULL;
    }

    data* sData = mmap(NULL, sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    close(shmFd);

    if(sData == MAP_FAILED) return NULL; // Failed to create the map

    // Main logic loop
    while(1){
        // Lock the mutex
        pthread_mutex_lock(&pool->tMutex);

        while(!pool->shutdown && IsQueueEmpty(&sData->queue))
            pthread_cond_wait(&pool->tCond, &pool->tMutex);
        
        if(pool->shutdown){
            pthread_mutex_unlock(&pool->tMutex);
            break;
        }

        // Get the filleSlots and queueMutex semaphores
        sem_t* filledSem = sem_open("/wsFilled", O_RDWR); 
        sem_t* queueMutex = sem_open("/wsQueueMutex", O_RDWR);

        // Lock the semaphores
        sem_wait(filledSem);
        sem_wait(queueMutex);

        int clientFd = sockDequeue(&sData->queue);

        // Unlock semaphores
        sem_post(filledSem);
        sem_post(queueMutex);

        // TODO HTTP logic

        pthread_mutex_unlock(&pool->tMutex);
    }
}