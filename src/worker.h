#ifndef _WORKER_
#define _WORKER_

#include "semaphores.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125624
        Guilherme Mendes Martins NMEC 125260
*/

// Main ROUTINE for the worker threads
void* workerThread(void* arg);

#endif