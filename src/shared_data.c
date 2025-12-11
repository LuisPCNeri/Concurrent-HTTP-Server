#include "shared_data.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

data* createSharedData(){
    // Create shared memory for read or write
    int shm_fd = shm_open("/web_server_shm", O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1) return NULL; // Failed to create shm

    // Try to set shared memory size to size of data
    if (ftruncate(shm_fd, sizeof(data)) == -1) {
        close(shm_fd);
        return NULL;
    }

    // CREATE shared map with size of data with ability to read or write to using the shm file descriptor
    data* sData = mmap(NULL, sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); 
    close(shm_fd);

    if(sData == MAP_FAILED) {
        perror("MAP");
        return NULL;
    }; // MAP CREATION FAILED

    memset(sData, 0, sizeof(data));

    // Instancialize queue variables
    sData->queue.head = 0;
    sData->queue.tail = 0;
    sData->queue.size = 1;

    sData->sem = (semaphore*) malloc(sizeof(semaphore));
    initSemaphores(sData->sem, 100);


    sData->cache = (cache*) malloc(sizeof(cache)); 
    createCache(sData->cache);

    // Create socket pair
    if(socketpair(AF_UNIX, SOCK_DGRAM, 0, sData->sv)  == -1) perror("Socket pair: ");

    return sData;
}

data* getSharedData(char* name){
    // Open shared memory segment to use
    int shmFd = shm_open(name, O_RDWR, 0666);
    if(shmFd == -1) return NULL;    // Failed to create shm

    // Try to set shared memory size to size of data
    if (ftruncate(shmFd, sizeof(data)) == -1) {
        close(shmFd);
        return NULL;
    }

    // Set up a pointer to mapped region
    data* sData = mmap(NULL, sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    close(shmFd);

    if(sData == MAP_FAILED){
        perror("MAP");
        return NULL;
    }; // Failed to create the map

    return sData;
}

void destroySharedData(data* sData){
    destroySemaphores(sData->sem);
    destroyCache(sData->cache);
    free(sData->sem);
    // UNMAP the sData variable
    munmap(sData, sizeof(data));
    // REMOVE shm link
    shm_unlink("/webServer_shm");
}

void sockEnqueue(connectionQueue* q, int fd){
    // Add file descriptor to queue
    q->socketQueue[q->tail] = fd;

    q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
    q->size++;
}

int sockDequeue(connectionQueue* q){
    // Take the first connnection in queue
    int clientFd = q->socketQueue[q->head];

    // Set head index to new value
    q->head = (q->head + 1) % MAX_QUEUE_SIZE;
    q->size--;

    return clientFd;
}

int IsQueueEmpty(connectionQueue* q){
    // If tail and head are in the same position queue is empty
    return q->tail == q->head;
}
