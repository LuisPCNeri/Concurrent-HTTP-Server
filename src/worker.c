#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

#include "shared_data.h"
#include "threadPool.h"
#include "worker.h"
#include "http.h"
#include "semaphores.h"
#include "logger.h"

unsigned int keepRunning = 1;

void INTHandler(int){
    keepRunning = 0;
}

void* workerThread(void* arg){
    signal(SIGINT, INTHandler);
    // Convert argument passed on thread create in threadPool.c to 
    threadPool* pool = (threadPool*) arg;
    semaphore* sems = pool->sems;
    
    data* sData = getSharedData("/web_server_shm");

    // Main logic loop
    while(keepRunning){
        if (sem_wait(sData->sem->filledSlots) != 0) {
            // This can happen if the semaphore wait is interrupted by a signal.
            // The loop condition will then handle shutdown.
            continue;
        }

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

        ssize_t rc = recvmsg(sData->sv[1], &cMsg, 0);

        if(rc < 0){
            perror("recvmsg failed");
            sem_post(sData->sem->emptySlots);
            continue;
        }


        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&cMsg);
        clientFd = (int*) CMSG_DATA(cmsg);
        

        if (cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS) {
            printf("The first control structure contains no file descriptor.\n");
            continue;
        }

        printf("Serving %d ON process %d\n", *clientFd, getpid());

        char buffer[4096];
        // Receive the http request
        ssize_t bytesRead = recv(*clientFd, buffer, sizeof(buffer) - 1, 0);
        serverLog(sData, "Received request");
        if(bytesRead == -1){
            perror("RECEIVED");
            sem_post(sems->queueMutex);
            continue;
        }

        sem_wait(sData->sem->statsMutex);
        sData->stats.totalRequests++;
        sem_post(sData->sem->statsMutex);

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
            
            sem_post(sData->sem->emptySlots);
            continue;
        }

        cacheNode* node = NULL; // Initialize to NULL
        ssize_t totalByteSent = 0;
        ssize_t bytes = 0;
        
        // Lock with a semaphore to not have multiple threads all reading from cache at the same time
        sem_wait(sData->sem->cacheSem);
        if( ( node = cacheLookup(sData->cache, request->path)) != NULL ){
            sem_post(sData->sem->cacheSem);

            if(( bytes = send(*clientFd, node->header, strlen(node->header), 0) ) == -1) perror("SEND");
            totalByteSent += bytes;

            if(node->size > 0 && strcmp(request->method, "HEAD") != 0) 
                totalByteSent += send(*clientFd, node->content, node->size, 0);

            sem_wait(sData->sem->statsMutex);

            switch (node->status)
            {
            case 200:
                sData->stats.status200++;
                break;
            case 404:
                sData->stats.status404++;
                break;
            case 500:
                sData->stats.status500++;
                break;
            default:
                break;
            }

            sem_post(sData->sem->statsMutex);
        }else{
            // If file was not cached it'd be better the semaphore still gets posted
            sem_post(sData->sem->cacheSem);

            httpResponse* response = (httpResponse*)malloc(sizeof(httpResponse));
            sendHttpResponse(*clientFd, request, response);
            free(response);
        }

        free(request);

        sem_post(sData->sem->emptySlots);

        if(close(*clientFd) == -1) perror("CLOSE");
        serverLog(sData, "Closed connection");

        sem_wait(sData->sem->statsMutex);
        // Update stats
        sData->stats.activeConnetions--;
        sData->stats.bytesTransferred += totalByteSent;

        sem_post(sData->sem->statsMutex);

        serverLog(sData, "Sent data");
    }

    printf("I EXITED????\n");

    return NULL;
}