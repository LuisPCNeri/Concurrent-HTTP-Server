#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "logger.h"
#include "semaphores.h"
#include "shared_data.h"

void updateStatFile(data* sData){
    FILE* file;

    if(! (file = fopen("www/statFile.txt", "w"))) perror("FOPEN");
    // put indicator in the beggining of file
    rewind(file);
    fprintf(file, "%d,%ld,%ld,%ld,%ld,%ld", sData->stats.activeConnetions, sData->stats.bytesTransferred, sData->stats.status200,
        sData->stats.status404, sData->stats.status500, sData->stats.totalRequests);
    fflush(file);

    fclose(file);
}

int getLogSize(){
    FILE* file;

    if( !(file = fopen("access.log", "r"))) perror("FOPEN");

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    fclose(file);

    return size;
}

void serverLog(data* sData, char* text){
    FILE* fptr;
    time_t now;

    struct tm* tInfo;
    time(&now);
    tInfo = localtime(&now);

    // Wait for sempahore
    sem_wait(sData->sem->logMutex);

    long log_size = getLogSize();

    if (log_size > 10 * 1024 * 1024) rename("access.log", "access.log.1");

    // Critical region

    // Open file
    if( (fptr = fopen(LOG_PATH, "a")) == NULL) perror("LOG FILE: ");
    // Write to file
    fprintf(fptr, "%s\t @ %s", text, asctime(tInfo));

    fclose(fptr);

    // Leave critical region
    sem_post(sData->sem->logMutex);
}