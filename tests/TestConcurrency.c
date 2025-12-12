#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125264
        Guilherme Mendes Martins NMEC 125260
*/

#define GREEN "\033[32m"
#define RESET "\033[0m"
#define RED "\033[31m"

unsigned long int fail;
unsigned long int passed;

void *client_thread(void *arg) {
    
    pthread_mutex_t* mutex = (pthread_mutex_t*)arg;

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[4096] = "";
    char message[2048] = "";

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("SOCKET :(");
        return NULL;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("CONNECT :(");
        close(sock);
        
        // Critical region we want the number of tests passed/failed to be the right one
        // Only one thread can update the counters at a time

        pthread_mutex_lock(mutex);
        fail++;
        pthread_mutex_unlock(mutex);

        return NULL;
    }

    // SEND request to the server
    snprintf(message, sizeof(message), 
        "GET / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        "http://localhost:8080");

    if( send(sock, message, strlen(message), 0) < 0) fail++;

    // RECEIVE from server
    int valread = recv(sock, buffer, 1024, 0);

    // Critical region we want the number of tests passed/failed to be the right one
    // Only one thread can update the counters at a time
    pthread_mutex_lock(mutex);
    if (valread >= 0) {
        if (strncmp(buffer, "HTTP/1.1 200", 12) == 0 || strncmp(buffer, "HTTP/1.0 200", 12) == 0) passed++;
        else fail++;
    }
    else fail++;
    pthread_mutex_unlock(mutex);

    close(sock);
    return NULL;
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        printf(RED"NEEDS AT LEAST 1 ARGUMENT THAT SHOULD BE NUMBER OF CONCURRENT REQUESTS"RESET"\n");
        return 1;
    }

    int numConnections = atoi(argv[1]);
    if (numConnections <= 0) {
        printf(RED"CONNECTIONS MUST BE A POSITIVE NUMBER"RESET"\n");
        return 1;
    }

    pthread_t *threads = malloc(numConnections * sizeof(pthread_t));
    if (threads == NULL) {
        perror("Malloc Threads");
        return 1;
    }

    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (mutex == NULL) {
        perror("Malloc Mutex");
        return 1;
    }

    pthread_mutex_init(mutex, NULL);


    printf("--- Starting %d client threads ---\n", numConnections);

    // Create and start client threads
    for (long i = 0; i < numConnections; i++) {
        if (pthread_create(&threads[i], NULL, client_thread, (void *)mutex) != 0) {
            perror("Create Threads");
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < numConnections; i++) {
        pthread_join(threads[i], NULL);
    }
    
    free(threads);

    printf("=== All client threads finished ===\n");

    if(fail > 0) printf(RED"TEST FAILED: %ld"RESET"\n", fail);
    else printf(GREEN"TEST PASSED: %ld"RESET"\n", passed);

    return 0;
}