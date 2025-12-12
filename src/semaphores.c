#include "semaphores.h"
#include <fcntl.h>

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125264
        Guilherme Mendes Martins NMEC 125260
*/

int initSemaphores(semaphore* sems, int queueSize){
    sems->emptySlots =  sem_open(  "/ws_empty",          O_CREAT, 0666, queueSize);
    sems->filledSlots = sem_open(  "/ws_filled",         O_CREAT, 0666, 0);
    sems->statsMutex =  sem_open(  "/ws_stats_mutex",    O_CREAT, 0666, 1);
    sems->logMutex =    sem_open(  "/ws_log_mutex",      O_CREAT, 0666, 1);
    sems->cacheSem =    sem_open(  "/ws_cache_sem",      O_CREAT, 0666, 1);

    if (sems->emptySlots == SEM_FAILED || sems->filledSlots == SEM_FAILED || sems->statsMutex == SEM_FAILED || sems->logMutex == SEM_FAILED) {
        return -1;
    }

    return 0;
}

void destroySemaphores(semaphore* sems){
    sem_close(sems->emptySlots);
    sem_close(sems->filledSlots);
    sem_close(sems->logMutex);
    sem_close(sems->statsMutex);
    sem_close(sems->cacheSem);

    sem_unlink("/ws_empty");
    sem_unlink("/ws_filled");
    sem_unlink("/ws_stats_mutex");
    sem_unlink("/ws_log_mutex");
    sem_unlink("/ws_cache_sem");
}