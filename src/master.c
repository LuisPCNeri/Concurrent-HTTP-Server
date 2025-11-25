#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "semaphores.h"
#include "http.h"
#include "shared_data.h"
#include "master.h"

int createServerSocket(int port){
    // Created socket using IPv4 sockets (AF_INET) as TCP packets (SOCK_STREAM) without specifying a protocol
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Throws an error if socket stream creation fails
    if(socket_fd < 0){
        perror("Socket Failed.");
        exit(EXIT_FAILURE);
    }
    
    int sock_option = 1;
    // SETS SO_REUSEADDR to 1 to specify that bind() SHOULD allow reuse of local addresses
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &sock_option, sizeof(sock_option));

    struct sockaddr_in socket_addr = {0};
    // Specifies connetions will be IPv4
    socket_addr.sin_family = AF_INET;
    // To accept connections from ANY network interface
    socket_addr.sin_addr.s_addr = INADDR_ANY;
    // Converts from host to network byte order
    socket_addr.sin_port = htons(port);

    // Binds the socket stream to the address set before
    // Socket will LISTEN to any clients that try to connet to port 8080
    if( bind(socket_fd, (struct sockaddr *) &socket_addr, sizeof(socket_addr)) < 0){
        perror("Could not bin socket");
        exit(EXIT_FAILURE);
    }

    // Takes a MAXIMUM of 100 connetions as is set there
    if( listen(socket_fd, 100) < 0 ){
        perror("Failed on listen");
        exit(EXIT_FAILURE);
    }

    printf("HTTP server running on http://localhost:%d\n", port);

    return socket_fd;
}

int acceptConnection(int socketFd, data* sharedData, semaphore* sem){
    // TODO there should be a check to see if queue is full before accepting
    // If queue is full send 503 Server Failure response

    // If queue is not full
    // CONNECTION ACCPETED
    int clientFd = accept(socketFd, NULL, NULL);
    
    // Block the semaphore for access to the stats struct
    sem_wait(sem->statsMutex);
    // Update active conncetion count
    sharedData->stats.activeConnetions++;
    sem_post(sem->statsMutex);

    // Added connection in clientFd socket to the connection queue in shared data
    enqueueCon(clientFd, sharedData, sem);
}

// Adds a connection on socket clientFd to the connection queue in shared memory
static void enqueueCon(int clientFd, data* sharedData, semaphore* sem){
    // Locks the queque and empty slots semaphore so no other threads can use them
    sem_wait(sem->emptySlots);
    sem_wait(sem->queueMutex);

    sockEnqueue(&sharedData->queue, clientFd);

    // Unlocks the semaphores
    sem_post(sem->emptySlots);
    sem_post(sem->queueMutex);
}