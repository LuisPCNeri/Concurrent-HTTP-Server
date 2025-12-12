#ifndef _LOGGER_
#define _LOGGER_

#define LOG_PATH "access.log"

#include "shared_data.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125264
        Guilherme Mendes Martins NMEC 125260
*/

void serverLog(data* sData, const char* reqType, const char* path, int status, int bytesTransferred);

// Write server stats in shared data to a file to be accessed by the web interface
// Uses the most recent stats in the shared memory segment to write to www/statFile.txt
void updateStatFile(data* sData);

#endif