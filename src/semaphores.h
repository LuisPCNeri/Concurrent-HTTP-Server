#ifndef _SEMAPHORES_H_
#define _SEMAPHORES_H_

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125624
        Guilherme Mendes Martins NMEC 125260
*/

#include <semaphore.h>
typedef struct
{
    sem_t* emptySlots;
    sem_t* filledSlots;
    sem_t* statsMutex;
    sem_t* logMutex;
    sem_t* cacheSem;
} semaphore;

// Initializes all the semaphores to their default value
int initSemaphores(semaphore* sem, int queueSize);

// Closes and Unlinks the sempahores
void destroySemaphores(semaphore* sem);

#endif