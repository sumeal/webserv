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
    static int create_listening_socket(int port);
    static int set_non_blocking(int fd);
    
};

#endif
