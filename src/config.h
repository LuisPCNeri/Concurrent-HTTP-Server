#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125624
        Guilherme Mendes Martins NMEC 125260
*/

typedef struct{
    int PORT;
    char DOC_ROOT[256];
    int WORKER_NUM;
    int THREAD_PER_WORKER;
    int MAX_QUEUE_CONF_SIZE;
    char LOG_FILE[256];
    int CACHE_SIZE_MB;
    int TIMEOUT_SEC;
} serverConf;

// LOADS data from config file int a serverConf object
// RETURNS 0 on success and -1 on failure
int loadConfig(const char* fileName, serverConf* conf);

#endif