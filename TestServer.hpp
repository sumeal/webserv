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
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <errno.h>
 #include <dirent.h>
 

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
		std::map<int, HttpRequest> client_req;
		std::map<int, std::string> client_buffer;
		void accepter();
		void handler(int client_fd);
		void responder(int client_fd);
		Server server_config;
		Location temp_location;
	    int new_socket;
		void handle_get_request(int client_fd, const HttpRequest& request, const std::string& location);
		void handle_post_request(int client_fd, const HttpRequest& request, const std::string& location);
		void handle_delete_request(int client_fd, const HttpRequest& request, const std::string& location);
		void send_error_response(int client_fd, int error_code, const std::string& message);
		void send_file_response(int client_fd, const std::string& file_path);
		bool is_method_allowed(const std::string& method, const std::string& location_path);
		void send_directory_listing(int client_fd,const std::string& file_path, const std::string& request_path);
		static bool has_path_travelsal(const std::string& p);

	public:
		std::vector<pollfd> pfds;
		void parse_http_request(const std::string &raw_req, int client_fd);
		void parse_config(const std::string& filename);
		std::string find_matching_location(const std::string& path);
		void print_config() const;
		TestServer();
		void launch();
		int set_non_blocking(int fd);

};

#endif