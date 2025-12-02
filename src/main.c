#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared_data.h"
#include "semaphores.h"
#include "threadPool.h"
#include "worker.h"
#include "master.h"
#include "http.h"
#include "config.h"

semaphore* sem;
// Socket Pair
int sv[2];

static pid_t createForks(int nForks, serverConf* conf){
    pid_t pid;
    pid_t parentId;
    if(nForks > 0){
        if( (pid = fork()) < 0) perror("Error creating separate processes :(");
        else if(pid == 0){
            // CHILD PROCESS
            printf("Created child process %d\n", getpid());
            close(sv[0]);

            threadPool* pool = CreateThreadPool(conf->THREAD_PER_WORKER, sem);

            DestroyThreadPool(pool);
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
    // TODO Add options to program

    serverConf* conf = (serverConf*) malloc(sizeof(serverConf));
    if( loadConfig("server.conf", conf) == -1) printf("Error loading config file.\n");
    
    // Init shared data
    data* sharedData = createSharedData(sv);

    // Init semaphores

    pid_t parentId = createForks(3, conf);
    
    if(getpid() == parentId){
        sleep(1);
        startStatsShow(sharedData);

        sem_post(sharedData->sem->emptySlots);
        sem_post(sharedData->sem->queueMutex);

        close(sv[1]);

        printf("SOCKET PORT: %d\n", conf->PORT);
        int socketFd = createServerSocket(conf->PORT);

        while(1){
            acceptConnection(socketFd, sharedData);
        }        
    }
}