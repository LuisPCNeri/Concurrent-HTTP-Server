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
            sem_post(sems->queueMutex);
            continue;
        }
        printf("BYTES READ %d\n", (int) bytesRead);

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

            continue;
        }

        cacheNode* node = (cacheNode*) malloc(sizeof(cacheNode));
        ssize_t totalByteSent = 0;
        ssize_t bytes = 0;

        /*if( ( node = cacheLookup(sData->cache, request->path)) != NULL ){
            printf("TWAS IN CACHE\n");

            if(( bytes = send(*clientFd, node->header, strlen(node->header), 0) ) == -1) perror("SEND");
            totalByteSent += bytes;

            if(strlen(node->content) > 0 && strcmp(request->method, "HEAD") != 0) 
            totalByteSent += send(*clientFd, node->content, strlen(node->content), 0);
        }else{*/
            httpResponse* response = (httpResponse*)malloc(sizeof(httpResponse));
            sem_wait(sData->sem->filledSlots);
            sendHttpResponse(*clientFd, request, response);
            sem_post(sData->sem->emptySlots);
            free(response);
        //}

        free(node);
        free(request);

        pthread_mutex_unlock(&pool->tMutex);

        if(close(*clientFd) == -1) perror("CLOSE");
        serverLog("Closed connection");

        sem_wait(sData->sem->statsMutex);
        // Update stats
        sData->stats.activeConnetions--;
        sData->stats.bytesTransferred += totalByteSent;

        sem_post(sData->sem->statsMutex);

        serverLog("Sent data");

        pthread_mutex_unlock(&pool->tMutex);

        // TODO Decrease active connection counter
    }

    printf("I EXITED????\n");

    return NULL;
}