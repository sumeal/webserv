#include "BindingSocket.hpp"

// Constructor
BindingSocket::BindingSocket(int domain, int service, int protocol, int port, u_long interface) : SimpleSocket(domain, service, protocol, port, interface)
{
	set_connection(connect_to_network(get_sock(), get_address()));
	test_connection(get_connection());
}

// Defination of connect_to_netowrk virtual fucntion
int BindingSocket::connect_to_network(int sock, struct sockaddr_in address)
{
	binding = bind(sock, (struct sockaddr *)&address, sizeof(address));
	return (binding);
}

int BindingSocket::get_binding()
{
	return (binding);
}