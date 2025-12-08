#ifndef WEBSERV_HPP
#define WEBSERV_HPP

# include <stdio.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>

class SimpleSocket
{
	private:
		struct sockaddr_in address;
		int sock;
		int connection;

	public:
		// Constructor
		SimpleSocket(int domain, int service, int protocol, int port, u_long interface);
		// Virtual fcuntion to connect to a network
		virtual int connect_to_network(int sock, struct sockaddr_in address) = 0;
		// Function to test sockets and connections
		void test_connection(int );
		// Getter fucntions
		struct sockaddr_in get_address();
		int get_sock();
		int get_connection();
};

#endif