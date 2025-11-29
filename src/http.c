#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <http.h>

#include "http.h"
#include "config.h"

#define PORT 8080
#define BUFFER_SIZE 4096

serverConf* conf = NULL;

int parseHttpRequest(const char* buffer, httpRequest* request){
    if(!conf){
        conf = (serverConf*)malloc(sizeof(serverConf));
        loadConfig("server.conf", conf);
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
    
    if( strcmp(request->path, "/") == 0 ) strcpy(request->path, "/index.html");

    return 0;
}

static char* getFileBody(const char* fileName, httpResponse* response){
    FILE* fptr;
    char buffer[4096];
    char line[256];

    if( !(fptr = fopen(fileName, "r")) ) perror("OPEN FILE");

    int c, i = 0;
    while( fgets(line, sizeof(line), fptr) ){
        strncat(buffer, line, sizeof(line));
    }

    strncpy(response->responseBody, buffer, sizeof(buffer));
    response->bodyLen = sizeof(buffer);

    return buffer;
}

static int getFileHeader(char* fileName, httpResponse* response, httpRequest* request){
    FILE* fptr;
    char fHeader[512];

    strcpy(request->path, strcat(conf->DOC_ROOT, request->path));
    printf("PATH %s\n", request->path);

    if( !(fptr = fopen(request->path, "rb"))){
        perror("Open File: ");
        printf("SERVING 404 ERROR...\n");
        
        response->status = 404;
        strcpy(response->statusMessage, "FILE NOT FOUND");
        strcpy(response->contentType, "text/html");
        
        
        strcpy(request->path, "www/404.html");

        return 0;
    }

    // TODO CHECK FOR 503 ERROR

    unsigned int fSize;

    // Get file size and return to start of file
    fseek(fptr, 0, SEEK_END);
    fSize = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    char* ext;

    const char* dot = strrchr(fileName, '.');
    if(!dot || dot == fileName) printf("NOT good file extension.\n");

    ext = (char*) malloc( sizeof( *(dot + 1)));
    strcpy(ext, dot + 1);

    if( strcmp(ext, "html") == 0 )      strcpy(response->contentType, "text/html");
    else if ( strcmp(ext, "js") == 0 )  strcpy(response->contentType, "text/js");
    else if ( strcmp(ext, "css") == 0 ) strcpy(response->contentType, "text/css");

    response->status = 200;
    strcpy(response->statusMessage, "OK");

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

    if(send(clientFd, header, header_len, 0) == -1) perror("SEND");
    if(response->responseBody && response->bodyLen > 0 && strcmp(request->method, "HEAD") != 0) 
        send(clientFd, response->responseBody, response->bodyLen, 0);
}