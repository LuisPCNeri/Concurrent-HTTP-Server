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
#include "serverCache.h"

#define PORT 8080
#define BUFFER_SIZE 4096

extern serverConf* config; // Use extern to link to the one in main.c
extern data* sData;

int parseHttpRequest(const char* buffer, httpRequest* request){
    char* line_end = strstr(buffer, "\r\n");
    if (!line_end) return -1;

    char first_line[1024];

    size_t len = line_end - buffer;
    strncpy(first_line, buffer, len);
    first_line[len] = '\0';

    // Use width specifiers in sscanf to prevent buffer overflows
    if (sscanf(first_line, "%15s %511s %15s", request->method, request->path, request->version) != 3) {
        return -1;
    }
    
    // Check if the combined path length will exceed the buffer before concatenation
    size_t root_len = strlen(config->DOC_ROOT);
    size_t path_len = strlen(request->path);
    if (root_len + path_len >= sizeof(request->path)) {
        fprintf(stderr, "Error: Requested URI is too long.\n");
        // Return an error to indicate a bad request
        return -1;
    }

    //printf("PATH BEFORE CHANGE %s\n", request->path);
    if( strcmp(request->path, "/") == 0 && strlen(request->path) < 2 ) strcpy(request->path, "/index.html");

    char full_path[sizeof(request->path)];
    // Safely construct the full path without modifying the original root
    snprintf(full_path, sizeof(full_path), "%s%s", config->DOC_ROOT, request->path);

    //printf("PATH %s\n\n", request->path);
    strncpy(request->path, full_path, sizeof(request->path) - 1);
    request->path[sizeof(request->path) - 1] = '\0'; // Ensure null-termination

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

    // TODO CHECK FOR 503 ERROR

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
    else strcpy(response->contentType, "application/octet-stream"); // Default for unknown types

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

        
    free(response->responseBody);

    // If file is not in the cache add it
    /*if( cacheLookup(sData->cache, request->path) == NULL ) {
        printf("CREATING ENTRY...\n");
        
        sem_wait(sData->sem->cacheSem);
        cacheInsert(sData->cache, request->path, header, response->responseBody);
        sem_post(sData->sem->cacheSem);
    }*/
    // Update total bytes sent stat
    sem_wait(sData->sem->statsMutex);
    sData->stats.bytesTransferred += totalByteSent;
    sem_post(sData->sem->statsMutex);
}