#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

#define MAX_QUEUE_SIZE 100

#include "semaphores.h"
#include "serverCache.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125624
        Guilherme Mendes Martins NMEC 125260
*/

typedef struct{
    long totalRequests;
    long bytesTransferred;
    long status200;
    long status404;
    long status5xx;
    int activeConnetions;
} serverStats;

typedef struct{
    serverStats stats;
    semaphore* sem;
    cache* cache;
    int sv[2];
} data;

// Creates a mapped region with data using sv[2] as a socket pair to communicate between processes
// Returns a pointer to the shared memory segment just created
data* createSharedData();

// Gets the data with NAME name
// Returns a pointer to the shared memory segment with NAME name or a NULL pointer if there was no match
data* getSharedData(char* name);
void destroySharedData(data*);

#endif