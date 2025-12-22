#ifndef TESTSERVER_HPP
#define TESTSERVER_HPP

 #include "SimpleServer.hpp"
 #include <unistd.h>
 #include <string.h>
 #include <fcntl.h>
 #include <poll.h>
 #include <vector>
 #include <map>

struct client_info
{
	int socket_fd;
};


struct Location
{
	std::string path;
	std::vector<std::string> cgi_ext;
	std::string cgi_path;
	bool auto_index;
};

struct Server
{
	std::string server_name;
	int port;
	std::string root;
	std::vector<std::string> index_files;
	std::map<int, std::string> error_pages;
};

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
		void parse_config(const std::string& filename);
		TestServer();
		void launch();

};

#endif