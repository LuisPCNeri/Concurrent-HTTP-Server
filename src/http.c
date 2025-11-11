#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080
#define BUFFER_SIZE 4096

const char* response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html><body><h1>Hello, World! HEHEHEHA</h1></body></html>";

// Sends file in cosnt char* path to int client_fd
void send_file(int client_fd, const char* path){
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

int main(void){

    // Created socket using IPv4 sockets (AF_INET) as TCP packets (SOCK_STREAM) without specifying a protocol
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Throws an error if socket stream creation fails
    if(socket_fd < 0){
        perror("Socket Failed.");
        exit(EXIT_FAILURE);
    }
    
    int sock_option = 1;
    // SETS SO_REUSEADDR to 1 to specify that bind() SHOULD allow reuse of local addresses
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &sock_option, sizeof(sock_option));

    struct sockaddr_in socket_addr = {0};
    // Specifies connetions will be IPv4
    socket_addr.sin_family = AF_INET;
    // To accept connections from ANY network interface
    socket_addr.sin_addr.s_addr = INADDR_ANY;
    // Converts from host to network byte order
    socket_addr.sin_port = htons(PORT);

    // Binds the socket stream to the address set before
    // Socket will LISTEN to any clients that try to connet to port 8080
    if( bind(socket_fd, (struct sockaddr *) &socket_addr, sizeof(socket_addr)) < 0){
        perror("Could not bin socket");
        exit(EXIT_FAILURE);
    }

    // Takes a MAXIMUM of 10 connetions as is set there
    if( listen(socket_fd, 10) < 0 ){
        perror("Failed on listen");
        exit(EXIT_FAILURE);
    }

    printf("HTTP server running on http://localhost:8080\n");

    // Infinite loop to ALWAYS listen for connections
    while(1){
        // Extracts the FIRST connection in the queque of pending connections
        int client_fd = accept(socket_fd, NULL, NULL);

        char buffer[BUFFER_SIZE];
        // Returns length of received message (received message FROM CLIENT) in bytes
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

        if(bytes_read > 0){
            buffer[bytes_read] = '\0';
            printf("Received: %s\n", buffer);
        }

        // SENDS const void * buf MESSAGE WITH size_t n LENGTH to int __fd SOCKET returns number of bytes sent
        send(client_fd, response, strlen(response), 0);

        send_file(client_fd, "/home/luisp/concurrent-http-server/test.txt");
        // Closes connection
        close(client_fd);
    }

    return 0;
}