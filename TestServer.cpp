#include "TestServer.hpp"

TestServer::TestServer() : SimpleServer(AF_INET, SOCK_STREAM, 0 ,8080, INADDR_ANY, 10)
{
	poll_setup();
	memset(buffer, 0, sizeof(buffer));
	launch();
}

void TestServer::poll_setup()
{
	pollfd pfd;
	pfd.fd = get_socket()->get_sock();
	pfd.events = POLLIN;
	pfds.push_back(pfd);
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
		int ready = poll(pfds.data(), pfds.size(), -1);
		if (ready < 0)
			perror("Poll error");
		for(size_t i = 0; i < pfds.size(); i++)
		{
			if (pfds[i].revents & POLLIN)
			{
				if (pfds[i].fd == get_socket()->get_sock())
				{
					accepter();
					pollfd new_pfd;
					new_pfd.fd = new_socket;
					new_pfd.events = POLLIN;
					pfds.push_back(new_pfd);
				}
				else
				{
					handler();
					responder();
					pfds.erase(pfds.begin() + i);
				}
			}
		}
	}
}