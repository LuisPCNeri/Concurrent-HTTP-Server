#include <sys/socket.h>
#include <netinet/in.h>
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

int main(int argc, char* argv[]){

    // Init shared data
    data* sharedData = createSharedData();

    // Init semaphores
    semaphore* sem;
    int sems = initSemaphores(sem, 100);

    createForks(3);
    
    int socketFd = createServerSocket(8080);

    while(1){
        int clientFd = acceptConnection(socketFd, sharedData, sem);
    }
}

static void createForks(int nForks){
    pid_t pid;
    if(nForks > 0){
        if( (pid = fork()) < 0) perror("Error creating separate processes :(");
        else if(pid == 0){
            // CHILD PROCESS
            CreateThreadPool(10);
        }
        else{
            // PARENTE PROCESS
            createForks(nForks - 1);
        }
    }
}