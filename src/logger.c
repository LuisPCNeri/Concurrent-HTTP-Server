#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "logger.h"
#include "semaphores.h"
#include "shared_data.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125624
        Guilherme Mendes Martins NMEC 125260
*/

void updateStatFile(data* sData){
    FILE* file;

    if(! (file = fopen("www/statFile.txt", "w"))) perror("FOPEN");
    // put indicator in the beggining of file
    rewind(file);
    fprintf(file, "%d,%ld,%ld,%ld,%ld,%ld", sData->stats.activeConnetions, sData->stats.bytesTransferred, sData->stats.status200,
        sData->stats.status404, sData->stats.status5xx, sData->stats.totalRequests);
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

void serverLog(data* sData, const char* reqType, const char* path, int status, int bytesTransferred){
    FILE* fptr;
    time_t now;

    // TIMESTAMP | REQUEST TYPE | PATH | STATUS | BYTES TRANSFERRED
    const char* LOG_FORMAT = "127.0.0.1 - - [%s -0800] %s %s HTTP/1.1 %d %d\n";

    struct tm* tInfo;
    time(&now);
    tInfo = localtime(&now);

    // To remove the trailing \n as it destroys the string
    char* time_str = asctime(tInfo);
    time_str[strcspn(time_str, "\n")] = 0;

    // Wait for sempahore
    // Critical region
    sem_wait(sData->sem->logMutex);

    long log_size = getLogSize();

    if (log_size > 10 * 1024 * 1024) rename("access.log", "access1.log");

    // Open file
    if( (fptr = fopen(LOG_PATH, "a")) == NULL) perror("LOG FILE: ");
    
    // Write to file
    fprintf(fptr, LOG_FORMAT, time_str, reqType, path, status, bytesTransferred);

    fclose(fptr);

    // Leave critical region
    sem_post(sData->sem->logMutex);
}