#ifndef _HTTP_
#define _HTTP_

typedef struct{
    char method[16];
    char path[512];
    char version[16];
} httpRequest;

typedef struct{
    int status;
    char statusMessage[256];
    char contentType[256];
    char* responseBody;
    size_t bodyLen;
} httpResponse;

// Creates the servers response to then send separatly to user
// Returns a pointer to an http response or a null pointer if it fails
httpResponse* createHttpResponse(httpRequest* request);

void sendHttpResponse(int clientFd, httpRequest* req, httpResponse* rep);

int parseHttpRequest(const char* buffer, httpRequest* request);

#endif