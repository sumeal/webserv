#include "TestServer.hpp"

TestServer::TestServer() : SimpleServer(AF_INET, SOCK_STREAM, 0 ,8080, INADDR_ANY, 10)
{
	memset(buffer, 0, sizeof(buffer));
	launch();
}

void TestServer::accepter()
{
	struct sockaddr_in address = get_socket()->get_address();
	int addrlen = sizeof(address);
	new_socket = accept(get_socket()->get_sock(), (struct sockaddr *)&address, (socklen_t *)&addrlen);
}

void TestServer::handler()
{
	read(new_socket, buffer, sizeof(buffer));
	std::cout << buffer << std::endl;
}

void TestServer::responder()
{
	const char *hello = "Hello from server!";
	write(new_socket, hello, strlen(hello));
	close(new_socket);
}

void TestServer::launch()
{
	while (true)
	{
		std::cout << "====Waiting====" << std::endl;
		accepter();
		handler();
		responder();
		std::cout << "====Done====" << std::endl;
	}
}