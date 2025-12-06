#ifndef _MASTER_
#define _MASTER_

#include "shared_data.h"
#include "semaphores.h"

typedef struct
{
    pthread_t* statsThread;
} master;


// Creates a socket for server to live in port passed as an argument. By default uses port 8080.
int createServerSocket(int port);
// Accepts a connection on a new socket and returns it's file descriptor.
int acceptConnection(int sokectFd, data* sharedData);

// Creates a new thread in the master process to show stats every 5 seconds
// Takes a data* pointer to pass to the thread start routine on pthread_create
// RETURNS 0 on SUCCESS and -1 on FAILURE
int startStatsShow(data* sharedData, master* m);

#endif