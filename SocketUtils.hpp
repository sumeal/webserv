#ifndef SOCKETUTILS_HPP
#define SOCKETUTILS_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstdio>  // For perror

class SocketUtils
{
public:
    // Create a listening socket with given port
    static int create_listening_socket(int port);
    
    // Set socket to non-blocking mode
    static int set_non_blocking(int fd);
    
    // Close socket safely
    static void close_socket(int fd);
};

#endif
