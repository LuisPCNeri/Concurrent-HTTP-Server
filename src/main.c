#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "shared_data.h"
#include "semaphores.h"
#include "threadPool.h"
#include "worker.h"
#include "master.h"
#include "http.h"
#include "config.h"

// COLORS :)
#define GREEN "\033[32m"
#define RESET "\033[0m"
#define RED "\033[31m"

semaphore* sem;
serverConf* config;
master* m;
data* sData; // Renamed to match usage in other files
// Socket Pair
int sv[2];

void INThandler(int);

static pid_t createForks(int nForks, serverConf* conf){
    pid_t pid;
    pid_t parentId;
    if(nForks > 0){
        if( (pid = fork()) < 0) perror("Error creating separate processes :(");
        else if(pid == 0){
            // CHILD PROCESS
            printf(GREEN "Created child process %d"RESET"\n", getpid());
            close(sData->sv[0]);

            // Initialize config and shared data pointer for this child process
            // This avoids race conditions from initializing in http.c
            config = (serverConf*) malloc(sizeof(serverConf));
            if( loadConfig("server.conf", config) == -1) printf("Error loading config file in child.\n");
            sData = getSharedData("/web_server_shm");

            threadPool* pool = CreateThreadPool(conf->THREAD_PER_WORKER, sem);

            DestroyThreadPool(pool);

            // Process as ended so exit gracefully :)
            exit(EXIT_SUCCESS);
        }
        else{
            // PARENTE PROCESS
            parentId = getpid();
            createForks(nForks - 1, conf);
        }
    }
    return parentId;
}

int main(void){
    signal(SIGINT, INThandler);
    // IGNORES sigpipe signal so that if client closes connection WHILST data is being sent the server does not just die
    
    // TODO Add options to program

    config = (serverConf*) malloc(sizeof(serverConf));
    if( loadConfig("server.conf", config) == -1) printf("Error loading config file.\n");

    m = (master*)calloc(1, sizeof(master));
    // Init shared data
    sData = createSharedData();

    // Init semaphores

    pid_t parentId = createForks(3, config);
    
    if(getpid() == parentId){
        sleep(1);
        startStatsShow(sData, m);

        sem_post(sData->sem->emptySlots);
        sem_post(sData->sem->queueMutex);

        close(sData->sv[1]);

        printf("SOCKET PORT: %d\n", config->PORT);
        int socketFd = createServerSocket(config->PORT);

        while(1){
            acceptConnection(socketFd, sData);
        }        
    }
}

void INThandler(int){
    free(config);

    if(m->statsThread != NULL){
        pthread_cancel(*m->statsThread);
        pthread_join(*m->statsThread, NULL);
        free(m->statsThread);
    }

    destroySharedData(sData);

    free(m);
    exit(EXIT_SUCCESS);

}
