#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <http.h>

#include "http.h"
#include "config.h"
#include "logger.h"
#include "shared_data.h"
#include "serverCache.h"

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125264
        Guilherme Mendes Martins NMEC 125260
*/

#define PORT 8080
#define BUFFER_SIZE 4096

extern serverConf* config; // Use extern to link to the one in main.c
extern data* sData;

int parseHttpRequest(const char* buffer, httpRequest* request){
    char* lineEnd = strstr(buffer, "\r\n");
    if (!lineEnd) return -1;

    char firstLine[1024];

    size_t len = lineEnd - buffer;
    strncpy(firstLine, buffer, len);
    firstLine[len] = '\0';

    // WIDTH specifiers to prevent overflow
    if (sscanf(firstLine, "%15s %511s %15s", request->method, request->path, request->version) != 3) {
        return -1;
    }
    
    if( strcmp(request->path, "/") == 0 ) strcpy(request->path, "/index.html");

    size_t rootLen = strlen(config->DOC_ROOT);
    size_t pathLen = strlen(request->path);
    // The +1 is for the null terminator
    if (rootLen + pathLen + 1 > sizeof(request->path)) {
        fprintf(stderr, "Error: Requested URI is too long.\n");
        // Return an error to indicate a bad request
        return -1;
    }

    // Prepend DOC_ROOT to the path. We need to move the existing path to make space.
    // +1 for null terminator
    memmove(request->path + rootLen, request->path, pathLen + 1);
    memcpy(request->path, config->DOC_ROOT, rootLen);

    return 0;
}

static int getFileBody(const char* fileName, httpResponse* response){
    FILE* fptr;

    // Open in binary read mode "rb"
    if( !(fptr = fopen(fileName, "rb")) ) {
        perror("OPEN FILE");
        return -1;
    }

    // Get the file size
    fseek(fptr, 0, SEEK_END);
    long fileSize = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    
    // ALLOC exactly as much space as the file needs
    response->responseBody = (char*) malloc(fileSize + 1);
    if (response->responseBody == NULL) {
        perror("malloc for response body failed");
        fclose(fptr);
        return -1;
    }

    // READ WHOLE file and put it into response body
    // Since fread returns size_t bytesRead set bodyLen to it's return value
    response->bodyLen = fread(response->responseBody, 1, fileSize, fptr);
    fclose(fptr);

    return 0;
}

static int getFileHeader(char* fileName, httpResponse* response, httpRequest* request){
    FILE* fptr;

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

    // CHECK FOR 503 ERROR
    if( strcmp(request->path, "www/503.html") == 0 ){
        // Setup httpResponse for 503 error
        response->status = 503;
        strcpy(response->statusMessage, "SERVICE UNAVAILABLE");
        strcpy(response->contentType, "text/html");

        // Set content type her to avoid the if statements bellow
        strcpy(response->contentType, "text/html");

        sem_wait(sData->sem->statsMutex);
        sData->stats.status5xx++;
        sem_post(sData->sem->statsMutex);

        return 0;
    }

    char* ext;

    char* dot = strrchr(fileName, '.');
    if(!dot || dot == fileName) printf("NOT good file extension.\n");

    ext = (dot + 1);

    if(       strcmp(ext, "html")== 0 )  strcpy(response->contentType, "text/html");
    else if ( strcmp(ext, "js")  == 0 )  strcpy(response->contentType, "application/javascript");
    else if ( strcmp(ext, "css") == 0 )  strcpy(response->contentType, "text/css");
    else if ( strcmp(ext, "png") == 0 )  strcpy(response->contentType, "image/png");
    else if ( strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)  strcpy(response->contentType, "image/jpeg");
    else if ( strcmp(ext, "pdf") == 0 )  strcpy(response->contentType, "application/pdf");
    // Default for unknown types
    else strcpy(response->contentType, "application/octet-stream"); 

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

    // USING MSG_NOSIGNAL AS TO NOT THROW SIGPIPE ERRORS (they would kill the process and that is bad)
    if(( bytes = send(clientFd, header, header_len, MSG_NOSIGNAL) ) == -1) perror("SEND");
    totalByteSent += bytes;
    if(response->bodyLen > 0 && strcmp(request->method, "HEAD") != 0) 
        totalByteSent += send(clientFd, response->responseBody, response->bodyLen, MSG_NOSIGNAL);

    // Log request information
    serverLog(sData,request->method, request->path, response->status, (int) totalByteSent);

    // If file is not in the cache add it
    sem_wait(sData->sem->cacheSem);
    if( cacheLookup(sData->cache, request->path) == NULL ) {
        cacheInsert(sData->cache, request->path, header, response->responseBody, response->bodyLen, response->status);
    }
    // The semaphore must always be posted, regardless of cache hit or miss.
    sem_post(sData->sem->cacheSem);

    free(response->responseBody);

    // Update total bytes sent stat
    sem_wait(sData->sem->statsMutex);
    sData->stats.bytesTransferred += totalByteSent;
    sem_post(sData->sem->statsMutex);
}