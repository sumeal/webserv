#include "webserv.hpp"

// Default Constructor

SimpleSocket::SimpleSocket(int domain, int service, int protocol, int port, u_long interface)
{
	//Define address structure
	address.sin_family = domain;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(interface);
	//Establish socket
	sock = socket(domain, service, protocol);
	//Test socket
	test_connection(sock);
}

// Test connection virtual function

void SimpleSocket::test_connection(int item_to_test)
{
	if (item_to_test < 0)
	{
		perror("Failed to connect...");
		exit(EXIT_FAILURE);
	}
}

struct sockaddr_in SimpleSocket::get_address()
{
	return (address);
}

int SimpleSocket::get_sock()
{
	return (sock);
}

int SimpleSocket::get_connection()
{
	return (connection);
}

void SimpleSocket::set_connection(int con)
{
	connection = con;
}