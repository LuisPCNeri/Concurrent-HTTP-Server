#ifndef _MASTER_
#define _MASTER_

#include "shared_data.h"
#include "semaphores.h"

// Creates a socket for server to live in port passed as an argument. By default uses port 8080.
int createServerSocket(int port);
// Accepts a connection on a new socket and returns it's file descriptor.
int acceptConnection(int sokectFd, data* sharedData, semaphore* sem, int* sv);

#endif