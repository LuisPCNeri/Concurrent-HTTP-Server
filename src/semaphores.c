#include "semaphores.h"
#include <fcntl.h>

int initSemaphores(semaphore* sems, int queueSize){
    sems->emptySlots = sem_open("/wsEmpty", O_CREAT);
    sems->filledSlots = sem_open("/wsFilled", O_CREAT);
    sems->queueMutex = sem_open("/wsQueueMutex", O_CREAT);
    sems->statsMutex = sem_open("/wsStatsMutex", O_CREAT);
    sems->logMutex = sem_open("/wsLogMutex", O_CREAT);

    if (sems->emptySlots == SEM_FAILED || sems->filledSlots == SEM_FAILED || sems->queueMutex == SEM_FAILED 
            || sems->statsMutex == SEM_FAILED || sems->logMutex == SEM_FAILED) {
        return -1;
    }

    return 0;
}

void destroySemaphores(semaphore* sems){
    sem_close(sems->emptySlots);
    sem_close(sems->filledSlots);
    sem_close(sems->logMutex);
    sem_close(sems->queueMutex);
    sem_close(sems->statsMutex);

    sem_unlink("/wsEmpty");
    sem_unlink("/wsFilled");
    sem_unlink("/wsQueueMutex");
    sem_unlink("/wsStatsMutex");
    sem_unlink("/wsLogMutex");  
}