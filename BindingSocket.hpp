#ifndef BINDINGSOCKET_HPP
#define BINDINGSOCKET_HPP

 #include "stdio.h"
 #include "webserv.hpp"

class BindingSocket: SimpleSocket
{
	public:
		// Constructor
		BindingSocket(int domain, int service, int protocol, int port, u_long interface);
		// Virtual fcuntion from parent
		int connect_to_network(int sock, struct sockaddr_in address);
};

#endif