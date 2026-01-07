#ifndef TESTSERVER_HPP
#define TESTSERVER_HPP

 #include "Parse.hpp"
 #include "SocketUtils.hpp"
 #include <unistd.h>
 #include <string.h>
 #include <fcntl.h>
 #include <poll.h>
 #include <vector>
 #include <map>
 #include <fstream>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <errno.h>
 #include <dirent.h>
 #include <iomanip>
 

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


class TestServer
{
	private:
		std::map<int, HttpRequest> client_req;
		std::map<int, std::string> client_buffer;
		Server server_config;
		Location temp_location;
	    int server_fd;     // Main listening socket
		int new_socket;    // New client connections
		void print_parsed_request(const HttpRequest& request, int client_fd) const;
		
		void accepter();
		void handler(int client_fd);
		void responder(int client_fd);      // PLACEHOLDER: For teammate integration

	public:
		std::vector<pollfd> pfds;
		char buffer[42000];
		
		void parse_config(const std::string& filename);
		
		void initialize_server();
		
		void parse_http_request(const std::string &raw_req, int client_fd);
		
		void launch();
		
		void print_config() const;
		
		TestServer();
		~TestServer();
};

#endif