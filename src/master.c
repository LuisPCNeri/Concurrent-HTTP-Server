#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "semaphores.h"
#include "http.h"
#include "shared_data.h"
#include "master.h"
#include "logger.h"


char* statsFormat = 
    "\n\n\nACTIVE CONNECTIONS: %d\r\n"
    "TOTAL BYTES TRANSFERRED: %d\r\n"
    "STATUS 200 COUNT: %d\r\n"
    "STATUS 404 COUNT: %d\r\n"
    "STATUS 500 COUNT: %d\r\n"
    "TOTAL REQUESTS COUNT: %d\n\n\n";

static void showStats(void* arg){
    data* sData = (data*) arg;

    while(1){
        sleep(5);

        printf(statsFormat, sData->stats.activeConnetions, sData->stats.bytesTransferred, sData->stats.status200, 
            sData->stats.status404, sData->stats.status500, sData->stats.totalRequests);
    }
}

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
        perror("Could not bind socket");
        exit(EXIT_FAILURE);
    }

    // Takes a MAXIMUM of 100 connetions as is set there
    if( listen(socket_fd, 100) < 0 ){
        perror("Failed on listen");
        exit(EXIT_FAILURE);
    }

    printf("============== HTTP server running on http://localhost:%d ==============\n", port);

    return socket_fd;
}

int acceptConnection(int socketFd, data* sharedData){
    // TODO there should be a check to see if queue is full before accepting
    // If queue is full send 503 Server Failure response

    int clientFd;
    // TODO Log Connection accepted
    if( (clientFd = accept(socketFd, NULL, NULL)) == -1 ){
        printf("ERROR\n");
    }
    serverLog("Accepted connection");

    sem_wait(sharedData->sem->emptySlots);
    sem_wait(sharedData->sem->queueMutex);

    char buf[1] = {0}; 
    struct iovec io = { .iov_base = buf, .iov_len = 1 };

    struct msghdr pMsg;
    memset(&pMsg, 0 ,sizeof(pMsg));
    struct cmsghdr* cmsg;
    char cmsgbuff[CMSG_SPACE(sizeof(clientFd))];
    pMsg.msg_control = cmsgbuff;
    pMsg.msg_controllen = sizeof(cmsgbuff);

    cmsg = CMSG_FIRSTHDR(&pMsg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(clientFd));

    *((int *) CMSG_DATA(cmsg)) = clientFd;

    pMsg.msg_controllen = cmsg->cmsg_len;

    pMsg.msg_iov = &io;
    pMsg.msg_iovlen = 1;

    if( sendmsg(sharedData->sv[0], &pMsg, 0) < 0) perror("Send message");

    close(clientFd);
    sem_post(sharedData->sem->queueMutex);
    sem_post(sharedData->sem->filledSlots);
    // If queue is not full
    // CONNECTION ACCPETED


    printf("Connection %d Accepted\n", clientFd);
    // Block the semaphore for access to the stats struct
    sem_wait(sharedData->sem->statsMutex);
    // Update active conncetion count
    sharedData->stats.activeConnetions++;
    sem_post(sharedData->sem->statsMutex);

    // Added connection in clientFd socket to the connection queue in shared data

    return 0;
}

int startStatsShow(data* sharedData){
    // Create new thread
    pthread_t* statsThread = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_create(statsThread, NULL, (void*) showStats, sharedData);

    return 0;
}