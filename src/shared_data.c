#include "shared_data.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

data* createSharedData(){
    // Create shared memory for read or write
    int shm_fd = shm_open("/webServer_shm", O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1) return NULL; // Failed to create shm

    // CREATE shared map with size of data with ability to read or write to using the shm file descriptor
    data* sData = mmap(NULL, sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); 
    close(shm_fd);

    if(sData = MAP_FAILED) return NULL; // MAP CREATION FAILED
    memset(sData, 0, sizeof(data));
    return sData;
}

void destroySharedData(data* sData){
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
