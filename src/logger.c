#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "logger.h"
#include "semaphores.h"
#include "shared_data.h"

void serverLog(char* text){
    FILE *fptr;
    time_t now;

    struct tm* tInfo;
    time(&now);
    tInfo = localtime(&now);

    // Get semaphores in shared memory
    data* sData = getSharedData("/web_server_shm");

    // Wait for sempahore
    sem_wait(sData->sem->logMutex);

    // Critical region

    // Open file
    if( (fptr = fopen(LOG_PATH, "a")) == NULL) perror("LOG FILE: ");
    // Write to file
    fprintf(fptr, "%s\t @ %s", text, asctime(tInfo));

    fclose(fptr);

    // Leave critical region
    sem_post(sData->sem->logMutex);
}