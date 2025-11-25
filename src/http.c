#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <http.h>

#include "http.h"

#define PORT 8080
#define BUFFER_SIZE 4096

const char* response =
    "HTTP/1.1 200 OK\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html><body><h1>Hello, World! HEHEHEHA</h1></body></html>";

// Sends file in cosnt char* path to int client_fd
void sendFile(int client_fd, const char* path){
    FILE* file = fopen(path, "rb");

    if(!file){
        // Send 404 Error to user
        const char* response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<h1>404 Not Found</h1>";
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Get file size
    // Goes to the END of the file
    fseek(file, 0, SEEK_END);
    // Returns the position of given stream in this case the END of file
    long file_size = ftell(file);
    // Puts the stream back at START of file
    fseek(file, 0, SEEK_SET);

    // Sends ONLY the file header

    char file_header[512];
    snprintf(file_header, sizeof(file_header), 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "\r\n", file_size);
    send(client_fd, file_header, strlen(file_header), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes;

    while( (bytes = fread(buffer, 1, sizeof(buffer), file)) > 0 ){
        // While any non zero number of bytes are being read
        send(client_fd, buffer, bytes, 0);
    }

    fclose(file);
}

int parseHttpRequest(const char* buffer, httpRequest* request){
    char* line_end = strstr(buffer, "\r\n");
    if (!line_end) return -1;

    char first_line[1024];

    size_t len = line_end - buffer;
    strncpy(first_line, buffer, len);
    first_line[len] = '\0';

    if (sscanf(first_line, "%s %s %s", request->method, request->path, request->version)!= 3) {
        return -1;
    }

    return 0;
}

void sendHttpResponse(int clientFd, int status, const char* statusMsg, const char* cType, const char* body, size_t bodyLen){
    char header[2048];

    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Server: ConcurrentHTTP/1.0\r\n"
        "Connection: close\r\n"
        "\r\n",
    status, statusMsg, cType, bodyLen);

    send(clientFd, header, header_len, 0);
    if(body && bodyLen > 0) send(clientFd, body, bodyLen, 0);
}