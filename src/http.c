#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <http.h>

#include "http.h"
#include "config.h"
#include "shared_data.h"

#define PORT 8080
#define BUFFER_SIZE 4096

serverConf* conf = NULL;
data* sData = NULL;

int parseHttpRequest(const char* buffer, httpRequest* request){
    if(!conf){
        conf = (serverConf*)malloc(sizeof(serverConf));
        loadConfig("server.conf", conf);
    }

    if(!sData){
        sData = getSharedData("/web_server_shm");
    }

    char* line_end = strstr(buffer, "\r\n");
    if (!line_end) return -1;

    char first_line[1024];

    size_t len = line_end - buffer;
    strncpy(first_line, buffer, len);
    first_line[len] = '\0';

    if (sscanf(first_line, "%s %s %s", request->method, request->path, request->version)!= 3) {
        return -1;
    }
    
    //printf("PATH BEFORE CHANGE %s\n", request->path);
    if( strcmp(request->path, "/") == 0 && strlen(request->path) < 2 ) strcpy(request->path, "/index.html");

    return 0;
}

static int getFileBody(const char* fileName, httpResponse* response){
    FILE* fptr;
    char buffer[4096] = "";
    char line[256] = "";

    if( !(fptr = fopen(fileName, "r")) ) {
        perror("OPEN FILE");

        return -1;
    }

    while( fgets(line, sizeof(line), fptr) )
        strcat(buffer, line);

    size_t len = strlen(buffer);
    memcpy(response->responseBody, buffer, len);

    response->bodyLen = len;

    //printf("SENT %s\n\n", response->responseBody);

    return 0;
}

static int getFileHeader(char* fileName, httpResponse* response, httpRequest* request){
    FILE* fptr;
    char* root = (char *)malloc(sizeof(char)*512);
    strcpy(root, conf->DOC_ROOT);

    //printf("PATH %s\n\n", request->path);
    strcpy(request->path, strcat(root, request->path));
    //printf("PATH %s\n\n", request->path);

    free(root);

    if( !(fptr = fopen(request->path, "rb"))){
        perror("Open File: ");

        // Update 404 stats
        sem_wait(sData->sem->statsMutex);
        sData->stats.status404++;
        sem_post(sData->sem->statsMutex);

        printf("SERVING 404 ERROR...\n");
        
        response->status = 404;
        strcpy(response->statusMessage, "FILE NOT FOUND");
        strcpy(response->contentType, "text/html");
        
        
        strcpy(request->path, "www/404.html");

        return 0;
    }

    // TODO CHECK FOR 503 ERROR

    char* ext;

    const char* dot = strrchr(fileName, '.');
    if(!dot || dot == fileName) printf("NOT good file extension.\n");

    ext = (char*) malloc( sizeof( *(dot + 1)));
    strcpy(ext, dot + 1);

    if( strcmp(ext, "html")      == 0 )  strcpy(response->contentType, "text/html");
    else if ( strcmp(ext, "js")  == 0 )  strcpy(response->contentType, "application/javascript");
    else if ( strcmp(ext, "css") == 0 )  strcpy(response->contentType, "text/css");
    else if ( strcmp(ext, "png") == 0 )  strcpy(response->contentType, "image/png");
    else if ( strcmp(ext, "jpg") == 0 )  strcpy(response->contentType, "image/jpeg");
    else if ( strcmp(ext, "pdf") == 0 )  strcpy(response->contentType, "application/pdf");

    response->status = 200;
    strcpy(response->statusMessage, "OK");

    // Update 200 stats
    sem_wait(sData->sem->statsMutex);
    sData->stats.status200++;
    sem_post(sData->sem->statsMutex);

    fclose(fptr);

    return 0;
}

void sendHttpResponse(int clientFd, httpRequest* request, httpResponse* response){
    char header[2048];

    getFileHeader(request->path, response, request);
    getFileBody(request->path, response);
    //TODO Add response stats to shared memory stats

    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Server: ConcurrentHTTP/1.0\r\n"
        "Connection: close\r\n"
        "\r\n",
    response->status, response->statusMessage, response->contentType, response->bodyLen);

    ssize_t totalByteSent = 0;
    ssize_t bytes = 0;

    if(( bytes = send(clientFd, header, header_len, 0) ) == -1) perror("SEND");
    totalByteSent += bytes;
    if(response->bodyLen > 0 && strcmp(request->method, "HEAD") != 0) 
        totalByteSent += send(clientFd, response->responseBody, response->bodyLen, 0);

    // Update total bytes sent stat
    sem_wait(sData->sem->statsMutex);
    sData->stats.bytesTransferred += totalByteSent;
    sem_post(sData->sem->statsMutex);
}