#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>

#include "shared_data.h"
#include "semaphores.h"
#include "threadPool.h"
#include "worker.h"
#include "master.h"
#include "http.h"
#include "config.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125264
        Guilherme Mendes Martins NMEC 125260
*/

// COLORS :)
#define GREEN "\033[32m"
#define RESET "\033[0m"
#define RED "\033[31m"
#define YELLOW "\033[33m"

semaphore* sem;
serverConf* config;
master* m;
data* sData;

char confPath[512] = "server.conf";

// Socket Pair
int sv[2];

void INThandler(int);

// Print the help message for the -h option
static void showHelp(){
    printf( "Usage: ./server [OPTIONS]\r\n"
            "Options:\r\n"
            "-c, --config PATH  \t   Configuration file path (default: ./server.conf)\r\n"
            "-p, --port PORT    \t   Port to listen on (default: 8080)\r\n"
            "-w, --workers NUM  \t   Number of worker processes (default: 4)\r\n"
            "-t, --threads NUM  \t   Threads per worker (default: 10)\r\n"
            "-h, --help         \t   Show this help message\r\n"
            "-v, --version      \t   Show version information\r\n"
           YELLOW "If no options are given the program will run with the configurations present in the server.conf file." RESET "\r\n");
}

// Print the version information for the -v option
static void showVersion(){
    printf(
        "\n=============================================================\r\n"
        "Project made for the operating systems class in DETI UA 25/26\r\n"
        "Authors: Luís Correia NMEC 125264 || Guilherme Martins 125260\r\n"
        "PROJECT VERSION: 1.0\r\n"
        "=============================================================\r\n\n"
    );
}

// This function will handle all the options given to main
// If there were any options given this function overwrites what is in the config object to reflect the changes the user made with the options
// If the -c option was used then it loads the new config file and saves it to the same config object effectively overwriting it
static void setOptions(int argc, char* argv[]){

    int opt;

    while( (opt = getopt(argc, argv, "c:p:w:t:hv")) != -1 ){
        switch(opt){
            case 'c':
                printf(YELLOW"CONF PATH: %s" RESET "\n", optarg);
                strcpy(confPath, optarg);

                // LOAD CONFIG AGAIN since it was loaded before and now it has a CUSTOM conf file
                if( loadConfig(confPath, config) == -1) {
                    printf(RED "Error loading config file. Check if file does exist." RESET"\n");
                    // Config loading as failed, user has been informed so exit with failure
                    // Most likely file does not exist
                    exit(EXIT_FAILURE);
                }

                break;
            case 'p':
                config->PORT = atoi(optarg);
                break;
            case 'w':
                config->WORKER_NUM = atoi(optarg);
                printf(YELLOW"WORKER NUM: %d" RESET "\n", config->WORKER_NUM);
                break;
            case 't':
                config->THREAD_PER_WORKER = atoi(optarg);
                printf(YELLOW"THREADS PER WORKER NUM: %d" RESET "\n", config->THREAD_PER_WORKER);
                break;
            case 'h':
                // Should show help
                showHelp();
                // DO NOT RUN THE SERVER IF USER JUST WANTS HELP
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                // Should show version
                showVersion();
                // DO NOT RUN THE SERVER IF USER JUST WANTS THE VERSION
                exit(EXIT_SUCCESS);
                break;
            default:
                break;
        }
    }
}

static pid_t createForks(int nForks, serverConf* conf){
    pid_t pid;
    pid_t parentId = 0;
    if(nForks > 0){
        if( (pid = fork()) < 0) perror("Error creating separate processes :(");
        else if(pid == 0){
            // CHILD PROCESS
            printf(GREEN "Created child process %d"RESET"\n", getpid());
            close(sData->sv[0]);

            // Initialize config and shared data pointer for this child process
            // This avoids race conditions from initializing in http.c
            config = (serverConf*) malloc(sizeof(serverConf));
            if( loadConfig(confPath, config) == -1) printf("Error loading config file in child.\n");
            sData = getSharedData("/web_server_shm");

            threadPool* pool = CreateThreadPool(conf->THREAD_PER_WORKER, sem);

            while(1) sleep(1);

            DestroyThreadPool(pool);

            // Process as ended so exit gracefully :)
            exit(EXIT_SUCCESS);
        }
        else{
            // PARENTE PROCESS
            parentId = getpid();
            createForks(nForks - 1, conf);
        }
    }
    return parentId;
}

int main(int argc, char* argv[]){
    signal(SIGINT, INThandler);
    signal(SIGTERM, INThandler);
    // IGNORES sigpipe signal so that if client closes connection WHILST data is being sent the server does not just die
    signal(SIGPIPE, SIG_IGN);

    config = (serverConf*) malloc(sizeof(serverConf));
    if( loadConfig(confPath, config) == -1) {
        printf(RED "Error loading config file. Check if file does exist." RESET"\n");
        // Config loading as failed, user has been informed so exit with failure
        // Most likely file does not exist
        exit(EXIT_FAILURE);
    }

    setOptions(argc, argv);

    m = (master*)calloc(1, sizeof(master));
    // Init shared data
    sData = createSharedData();

    pid_t parentId = createForks(config->WORKER_NUM, config);
    
    if(getpid() == parentId){
        startStatsShow(sData, m);

        sem_post(sData->sem->emptySlots);

        close(sData->sv[1]);

        int socketFd = createServerSocket(config->PORT);

        while(1){
            acceptConnection(socketFd, sData);
        }        
    }
}

// Cleanup function that intercepts SIGINT and SIGTERM
void INThandler(int){
    free(config);

    if(m->statsThread != NULL){
        pthread_cancel(*m->statsThread);
        pthread_join(*m->statsThread, NULL);
        free(m->statsThread);
    }

    destroySharedData(sData);

    free(m);
    exit(EXIT_SUCCESS);

}