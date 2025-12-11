#ifndef _SEMAPHORES_H_
#define _SEMAPHORES_H_

#include <semaphore.h>
typedef struct
{
    sem_t* emptySlots;
    sem_t* filledSlots;
    sem_t* queueMutex;
    sem_t* statsMutex;
    sem_t* logMutex;
    sem_t* cacheSem;
} semaphore;

int initSemaphores(semaphore* sem, int queueSize);
void destroySemaphores(semaphore* sem);

#endif