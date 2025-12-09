#ifndef TESTSERVER_HPP
#define TESTSERVER_HPP

 #include "SimpleServer.hpp"
 #include <unistd.h>
 #include <string.h>

class TestServer: public SimpleServer
{
	private:
		char buffer[30000];
		int new_socket;
		void accepter();
		void handler();
		void responder();
	public:
		TestServer();
		void launch();

};

#endif