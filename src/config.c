#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125264
        Guilherme Mendes Martins NMEC 125260
*/

int loadConfig(const char* fileName, serverConf* conf){
    FILE* fp = fopen(fileName, "r");
    if(!fp) return -1;

    char line[512], key[128], value[256];

    while(fgets(line, sizeof(line), fp)){
        // Line is empty so skip
        if(line[0] == '#' || line[0] == '\n') continue;

        // Load the information in the config file
        if (sscanf(line, "%[^=]=%s", key, value) == 2){
            if (strcmp(key, "PORT") == 0) conf->PORT = atoi(value);
            else if(strcmp(key, "DOCUMENT_ROOT") == 0) strncpy(conf->DOC_ROOT, value, sizeof(conf->DOC_ROOT));
            else if(strcmp(key, "NUM_WORKERS") == 0) conf->WORKER_NUM = atoi(value);
            else if(strcmp(key, "THREADS_PER_WORKER") == 0) conf->THREAD_PER_WORKER = atoi(value);
            else if(strcmp(key, "MAX_QUEUE_SIZE") == 0) conf->MAX_QUEUE_CONF_SIZE = atoi(value);
            else if(strcmp(key, "LOG_FILE") == 0) strncpy(conf->LOG_FILE, value, sizeof(conf->LOG_FILE));
            else if(strcmp(key, "CACHE_SIZE_MB") == 0) conf->CACHE_SIZE_MB = atoi(value);
            else if(strcmp(key, "TIMEOUT_SECONDS") == 0) conf->TIMEOUT_SEC = atoi(value);
        }
    }

    fclose(fp);
    return 0;
}