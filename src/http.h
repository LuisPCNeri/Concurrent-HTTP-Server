#ifndef _HTTP_
#define _HTTP_

/*
    AUTHORS:
        Luís Pedro Costa Néri Correia NMEC 125624
        Guilherme Mendes Martins NMEC 125260
*/

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

// Takes a fd to a socket where a client lives
// Uses the httpRequest object to create the response
// This function takes care of the ipc synchronization for the stats updates inside itself
// Sends the response to the client socket and updates the stats
void sendHttpResponse(int clientFd, httpRequest* req, httpResponse* rep);

int parseHttpRequest(const char* buffer, httpRequest* request);

#endif