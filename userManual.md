# Concurrent HTTP Server

## Authors

Luís Pedro Costa Néri Correia NMEC 125264<br>
Guilherme Mendes Martins NMEC 125260

## User Manual

### Compiling and building

To compile and build the server, use the Make file. The available commands are as follows:

- `make`: compiles and builds the server binary.
- `make run`: compiles, builds AND runs the server binary.
- `make test`: compiles, builds AND runs automated tests for the server.
- `make clear`: removes any executables and object files.

(*Note: object files will be stored on the obj directory, in this directory.*)

<hr>

### Configuring the server

Configuring the server is done by changing values in the `server.conf` file. Its structure and default values are as follows: 

``` 
PORT=8080
DOCUMENT_ROOT=www
NUM_WORKERS=4
THREADS_PER_WORKER=10
MAX_QUEUE_SIZE=100
LOG_FILE=access.log
CACHE_SIZE_MB=10
TIMEOUT_SECONDS=30
```

- `PORT`: specifies the port the server will use.
- `DOCUMENT_ROOT`: specifies the document root the server will use.
- `NUM_WORKERS`: specifies the number of worker processes that will be created when the server is started.
- `THREADS_PER_WORKER` specifies the number of threads each worker processes' thread pool will have.
- `MAX_QUEUE_SIZE` specifies the max size of the request queue.
- `LOG_FILE` specifies the path to the file that will be used to log server information.
- `CACHE_SIZE_MB` specifies the size of the server's cache, in megabytes.
- `TIMEOUT_SECONDS` specifies how long a server timeout will last, in seconds. 

<hr>

### Execution instructions