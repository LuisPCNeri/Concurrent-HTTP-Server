#ifndef _HTTP_
#define _HTTP_

typedef struct{
    char method[16];
    char path[512];
    char version[16];
} httpRequest;
// Creates and returns the file descpritor for the socket where the server lives PORT by default is 8080
void sendFile(int clientFd, const char* path);

void sendHttpResponse(int clientFd, int status, const char* statusMsg, const char* cType, const char* body, size_t bodyLen);

int parseHttpRequest(const char* buffer, httpRequest* request);

#endif