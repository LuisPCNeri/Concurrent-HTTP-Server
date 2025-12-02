#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "shared_data.h"
#include "threadPool.h"
#include "worker.h"
#include "http.h"
#include "semaphores.h"
#include "logger.h"

void* workerThread(void* arg){
    // Convert argument passed on thread create in threadPool.c to 
    threadPool* pool = (threadPool*) arg;
    semaphore* sems = pool->sems;
    
    data* sData = getSharedData("/web_server_shm");

    // Main logic loop
    while(1){
        // Lock the mutex
        pthread_mutex_lock(&pool->tMutex);

        if(pool->shutdown){
            printf("SHUTDOWN\n");
            break;
        }

        pthread_mutex_unlock(&pool->tMutex);

        // TODO Implement use of semaphores for thread safety (or equivalent)
        // Wait here for an empty

        int* clientFd;

        char msg_buffer[80];
        struct iovec iov[1];

        iov[0].iov_base = msg_buffer;
        iov[0].iov_len  = sizeof(msg_buffer);

        struct msghdr cMsg;
        memset(&cMsg, 0, sizeof(cMsg));
        char cmsgbuff[CMSG_SPACE(sizeof(int))];
        cMsg.msg_iov = iov;
        cMsg.msg_iovlen = 1;
        cMsg.msg_control = cmsgbuff;
        cMsg.msg_controllen = sizeof(cmsgbuff);

        pthread_mutex_lock(&pool->tMutex);

        ssize_t rc = recvmsg(sData->sv[1], &cMsg, 0);

        sem_post(sData->sem->emptySlots);

        if(rc < 0){
            perror("recvmsg failed");
            exit(-1);
        }


        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&cMsg);
        clientFd = (int*) CMSG_DATA(cmsg);
        

        if (cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS) {
            printf("The first control structure contains no file descriptor.\n");
            continue;
        }

        //int clientFd = sockDequeue(&sData->queue);
        printf("Serving %d ON process %d\n", *clientFd, getpid());

        char buffer[4096];
        // Receive the http request
        ssize_t bytesRead = recv(*clientFd, buffer, sizeof(buffer) - 1, 0);
        serverLog("Received request");
        if(bytesRead == -1){
            perror("RECEIVED");
            exit(-1);
            sem_post(sems->queueMutex);
            continue;
        }
        printf("BYTES READ %d\n", (int) bytesRead);
        if(bytesRead > 0){
            buffer[bytesRead] = '\0';
            //printf("Received: %s\n", buffer);
        }

        httpRequest* request = malloc(sizeof(httpRequest));
        if(parseHttpRequest(buffer, request) == -1){
            printf("NOT good request\n");
            free(request);
            close(*clientFd);

            sem_wait(sData->sem->statsMutex);
            // Update stats
            sData->stats.activeConnetions--;
            sem_post(sData->sem->statsMutex);

            continue;
        }

        httpResponse* response = (httpResponse*)malloc(sizeof(httpResponse));

        sem_wait(sData->sem->filledSlots);

        sendHttpResponse(*clientFd, request, response);

        sem_post(sData->sem->emptySlots);
        pthread_mutex_unlock(&pool->tMutex);

        free(response);
        free(request);

        pthread_mutex_unlock(&pool->tMutex);

        if(close(*clientFd) == -1) perror("CLOSE");
        serverLog("Closed connection");

        sem_wait(sData->sem->statsMutex);
        // Update stats
        sData->stats.activeConnetions--;
        sem_post(sData->sem->statsMutex);

        serverLog("Sent data");

        pthread_mutex_unlock(&pool->tMutex);

        // TODO Decrease active connection counter
    }

    printf("I EXITED????\n");

    return NULL;
}