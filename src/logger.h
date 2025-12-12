#ifndef _LOGGER_
#define _LOGGER_

#define LOG_PATH "access.log"

#include "shared_data.h"

void serverLog(data* sData, const char* reqType, const char* path, int status, int bytesTransferred);
// Write server stats in shared data to a file to be accessed by the web interface
void updateStatFile(data* sData);

#endif