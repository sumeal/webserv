#ifndef CONNECTINGSOCKET_HPP
#define CONNECTINGSOCKET_HPP

#include "webserv.hpp"

class ConnectingSocket: public SimpleSocket
{
	public:
		// Constructor
		ConnectingSocket(int domain, int service, int protocol, int port, u_long interface);
		// Virtual fucntion from parent
		int connect_to_network(int sock, struct sockaddr_in address);
}

#endif