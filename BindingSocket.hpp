#ifndef BINDINGSOCKET_HPP
#define BINDINGSOCKET_HPP

 #include "stdio.h"
 #include "webserv.hpp"

class BindingSocket: SimpleSocket
{
	public:
		BindingSocket(int domain, int service, int protocol, int port, u_long interface) : SimpleSocket(domain, service, protocol, port, interface);
		int connect_to_network(int sock, struct sockaddr_in address);
};

#endif