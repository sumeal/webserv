#include "SocketUtils.hpp"

int SocketUtils::create_listening_socket(int port)
{
    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // 2. Set socket options (reuse address)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_fd);
        return -1;
    }
    
    // 3. Setup address structure
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // 4. Bind socket to address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }
    
    // 5. Listen for connections (increase backlog for high concurrency)
    if (listen(server_fd, 128) < 0) {  // Increased from 10 to 128
        perror("Listen failed");
        close(server_fd);
        return -1;
    }
    
    std::cout << "Socket created and listening on port " << port << std::endl;
    return server_fd;
}

int SocketUtils::set_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void SocketUtils::close_socket(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}
