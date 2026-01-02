#ifndef TESTSERVER_HPP
#define TESTSERVER_HPP

 #include "SimpleServer.hpp"
 #include "Parse.hpp"
 #include <unistd.h>
 #include <string.h>
 #include <fcntl.h>
 #include <poll.h>
 #include <vector>
 #include <map>
 #include <fstream>

	class Parse;
 	class TestServer;  // Forward declaration
	struct Server;     // Forward declaration  
	struct Location;   // Forward declaration
	struct client_info
{
	int socket_fd;
};

enum ParseState {
	OUTSIDE,
	SERVER,
	LOCATION
};

struct Location
{
	std::string path;
	bool allow_get;
	bool allow_post;
	bool allow_delete;
	std::vector<std::string> cgi_ext;
	std::string cgi_path;
	std::string upload_path;
	bool auto_index;
};

struct Server
{
	std::string server_name;
	int port;
	std::string root;
	std::vector<std::string> index_files;
	std::map<int, std::string> error_pages;
	std::vector<Location> locations;
};

struct HttpRequest
{
    // Request line
    std::string method;          // "GET", "POST", "DELETE"
    std::string uri;             // "/uploads/file.txt?x=1"
    std::string path;            // "/uploads/file.txt"
    std::string query;           // "x=1"
    std::string http_version;    // "HTTP/1.1"

    // Headers
    std::map<std::string, std::string> headers;

    // Parsed headers (cached for convenience)
    std::string host;
    int         port;
    size_t      content_length;
    std::string content_type;
    bool        keep_alive;

    // Body
    std::string body;

    // State flags
    bool is_cgi;
};


class TestServer: public SimpleServer
{
	private:
		char buffer[30000];
		int new_socket;
		void accepter();
		void handler();
		void responder();
		Server server_config;
		Location temp_location;
	public:
		std::vector<pollfd> pfds;
		void parse_config(const std::string& filename);
		void print_config() const;
		TestServer();
		void launch();
		int set_non_blocking(int fd);

};

#endif