#ifndef TESTSERVER_HPP
#define TESTSERVER_HPP

 #include "SimpleServer.hpp"
 #include <unistd.h>
 #include <string.h>
 #include <fcntl.h>
 #include <poll.h>
 #include <vector>

class TestServer: public SimpleServer
{
	private:
		char buffer[30000];
		int new_socket;
		void accepter();
		void handler();
		void responder();
	public:
		std::vector<pollfd> pfds;
		TestServer();
		void launch();
		void poll_setup();

};

#endif