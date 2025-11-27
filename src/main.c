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

semaphore* sem;
// Socket Pair
int sv[2];

pid_t createForks(int nForks){
    pid_t pid;
    pid_t parentId;
    if(nForks > 0){
        if( (pid = fork()) < 0) perror("Error creating separate processes :(");
        else if(pid == 0){
            // CHILD PROCESS
            printf("Created child process %d\n", getpid());
            close(sv[0]);
            threadPool* pool = CreateThreadPool(10, sem);

            DestroyThreadPool(pool);
        }
        else{
            // PARENTE PROCESS
            parentId = getpid();
            createForks(nForks - 1);
        }
    }
    return parentId;
}

int main(void){
    // Create Socket Pair
    if(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv)  == -1) perror("Socket pair: ");
    // Init shared data
    data* sharedData = createSharedData(sv);

    // Init semaphores

    pid_t parentId = createForks(3);
    
    if(getpid() == parentId){
        sleep(1);

        sem_post(sharedData->sem->emptySlots);
        sem_post(sharedData->sem->queueMutex);

        close(sv[1]);
        int socketFd = createServerSocket(8080);

        while(1){
            acceptConnection(socketFd, sharedData, sem, sv);
        }        
    }
}