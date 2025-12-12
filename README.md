# Concurrent HTTP Server

## Authors

Luís Pedro Costa Néri Correia NMEC 125264<br>
Guilherme Mendes Martins NMEC 125260

## Description

This project consists of a HTTP server able to employ multi-threaded concurrency in order to parse HTTP requests, using the product-consumer pattern. The entirity of the server's code is written in the C programming language. Sample HTML pages to test server functionality are also present, with one of those pages displaying stats related to the server's functioning.    

## How to use

For information on how to use the server, read the user manual, in the markdown file `userManual.md`.

## References

    -https://pubs.opengroup.org/onlinepubs/7908799/xns/syssocket.h.html
    -https://dev.to/jeffreythecoder/how-i-built-a-simple-http-server-from-scratch-using-c-739
    -https://pubs.opengroup.org/onlinepubs/009695399/basedefs/netinet/in.h.html
    -https://pubs.opengroup.org/onlinepubs/007904875/basedefs/sys/mman.h.html
    -https://pubs.opengroup.org/onlinepubs/7908799/xsh/semaphore.h.html
    -https://stackoverflow.com/questions/29331938/what-things-are-exactly-happening-when-server-socket-accept-client-sockets
    -https://stackoverflow.com/questions/11461106/socketpair-in-c-unix
    -https://www.ibm.com/docs/en/ztpf/1.1.2025?topic=apis-socketpair-create-pair-connected-sockets
    -https://www.ibm.com/docs/en/i/7.6.0?topic=processes-example-worker-program-used-sendmsg-recvmsg
    -https://www.geeksforgeeks.org/c/memset-c-example/
    -https://stackoverflow.com/questions/8481138/how-to-use-sendmsg-to-send-a-file-descriptor-via-sockets-between-2-processes
    -https://www.quora.com/How-can-you-pass-file-descriptors-between-processes-in-Unix
    -https://stackoverflow.com/questions/1788095/descriptor-passing-with-unix-domain-sockets
    -https://stackoverflow.com/questions/14721005/connect-socket-operation-on-non-socket
    -https://stackoverflow.com/questions/2358684/can-i-share-a-file-descriptor-to-another-process-on-linux-or-are-they-local-to-t
    -https://www.ibm.com/docs/en/zos/3.2.0?topic=calls-sendmsg
    -https://linux.die.net/man/7/unix
    -https://linux.die.net/man/3/cmsg
    -https://www.ibm.com/docs/en/zos/2.5.0?topic=functions-sendmsg-send-messages-socket
    -https://stackoverflow.com/questions/8359322/how-to-share-semaphores-between-processes-using-shared-memory
    -https://mimesniff.spec.whatwg.org/#javascript-mime-type
    -https://www.reddit.com/r/learnjavascript/comments/r4ss8t/how_to_parse_data_from_txt_file_using_javascript/
    -https://pubs.opengroup.org/onlinepubs/007904975/functions/fread.html
    -https://linux.die.net/man/2/send
    -https://man7.org/linux/man-pages/man3/getopt.3.html
    -https://pubs.opengroup.org/onlinepubs/009696799/functions/getopt.html
    -https://serverfault.com/questions/146605/understanding-this-error-apr-socket-recv-connection-reset-by-peer-104
    -https://makefiletutorial.com/