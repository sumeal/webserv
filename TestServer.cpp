#include "TestServer.hpp"
#include "Parse.hpp"

TestServer::TestServer() : SimpleServer(AF_INET, SOCK_STREAM, 0 ,8080, INADDR_ANY, 10)
{	
	// Initialize config values
	server_config.port = 0;
	server_config.server_name = "";
	server_config.root = "";
	
	// Initialize temp_location
	temp_location.path = "";
	temp_location.allow_get = false;
	temp_location.allow_post = false;
	temp_location.allow_delete = false;
	temp_location.cgi_path = "";
	temp_location.auto_index = false;
}

void TestServer::parse_config(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: config file failed" << std::endl;
        return;
    }
    
    ParseState current_state = OUTSIDE;
    std::string line;

    while (std::getline(file, line)) {
        // Clean the line
        line = Parse::trim_line(line);
        
        // Skip comments and empty lines
        if (Parse::is_comment_or_empty(line))
            continue;
            
        std::cout << "DEBUG - State: " << current_state << " | Line: '" << line << "'" << std::endl;
        
        // Parse based on current state
        if (current_state == OUTSIDE) {
            current_state = (ParseState)Parse::parse_outside(line, current_state);
        }
        else if (current_state == SERVER) {
            int new_state = Parse::parse_server(line, current_state, server_config);
            
            // If entering location block, extract path
            if (new_state == LOCATION && current_state == SERVER) {
                std::vector<std::string> tokens = Parse::split_line(line);
                if (tokens.size() >= 2 && tokens[0] == "location") {
                    // Reset temp_location for new location
                    temp_location = Location();
                    temp_location.path = tokens[1];
                    temp_location.allow_get = false;
                    temp_location.allow_post = false;
                    temp_location.allow_delete = false;
                    temp_location.auto_index = false;
                    std::cout << "DEBUG - Starting location: " << temp_location.path << std::endl;
                }
            }
            current_state = (ParseState)new_state;
        }
        else if (current_state == LOCATION) {
            int new_state = Parse::parse_location(line, current_state, temp_location);
            
            // If leaving location block, store the location
            if (new_state == SERVER && current_state == LOCATION) {
                server_config.locations.push_back(temp_location);
                std::cout << "DEBUG - Stored location: " << temp_location.path << std::endl;
            }
            current_state = (ParseState)new_state;
        }
        
        std::cout << "DEBUG - New State: " << current_state << std::endl;
    }
    
    file.close();
    std::cout << "Config parsing completed" << std::endl;
    
    // Debug output
    std::cout << "DEBUG - Final values:" << std::endl;
    std::cout << "Port: " << server_config.port << std::endl;
    std::cout << "Server name: '" << server_config.server_name << "'" << std::endl;
    std::cout << "Root: '" << server_config.root << "'" << std::endl;
    std::cout << "Locations count: " << server_config.locations.size() << std::endl;
}

int TestServer::set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void TestServer::accepter()
{
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof (client_addr);

	new_socket = accept(get_socket()->get_sock(), (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);

	if (new_socket < 0) {
		perror("Accept fail");
		return;
	}
	if (set_non_blocking(new_socket) < 0) {
		perror("Failed to set non-blocking");
		return;
	}
	
}

void TestServer::handler(int client_fd)
{
	char buffer[42000];
	memset(buffer, 0, sizeof(buffer));
	ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer - 1));
	if (bytes_read <= 0) {
		std::cout << "Client disconnected or read error" << std::endl;
		client_req.erase(client_fd);
		client_buffer.erase(client_fd);
		return;
	}
	buffer[bytes_read] = '\0';
	client_buffer[client_fd] += std::string(buffer);
	if (client_buffer[client_fd].find("\r\n\r\n") != std::string::npos) {
		parse_http_request(client_buffer[client_fd], client_fd);
		responder(client_fd);

		client_req.erase(client_fd);
		client_buffer.erase(client_fd);
	}
}

void TestServer::parse_http_request(const std::string &raw_req, int client_fd)
{
	HttpRequest& current_request = client_req[client_fd];
	std::istringstream request_stream(raw_req);
	std::string line;

	if (std::getline(request_stream, line)) {
		std::istringstream req_line(line);
		req_line >> current_request.method >> current_request.uri >> current_request.http_version;

		size_t query_pos = current_request.uri.find('?');
		if (query_pos != std::string::npos) {
			current_request.path = current_request.uri.substr(0, query_pos);
			current_request.query = current_request.uri.substr(query_pos +1);
		}
		else {
			current_request.path = current_request.uri;
		}
	}

	while (std::getline(request_stream, line) && line != "\r" && !line.empty()) {
		size_t colon_pos = line.find(':');
		if (colon_pos != std::string::npos) {
			std::string header_name = line.substr(0, colon_pos);
			std::string header_value = line.substr(colon_pos + 2);

			if (!header_value.empty() && header_value[header_value.length() - 1] == '\r') {
				header_value.push_back();
			}
			current_request.headers[header_name] = header_value;
		}
	}
}

void TestServer::responder(int client_fd)
{
	try {
		std::map<int, HttpRequest>::iterator it = client_req.find(client_fd);
		if (it == client_req.end()) {
			throw std::runtime_error("No request found for client");
		}

		HttpRequest& request = it->second;
		std::string matched_location = find_matching_location(request.path);

		// if (is_cgi_request(request.path,matched_location)) {
		// 	handle_cgi_request(client_fd, request, matched_location);
		// 	return;
		// }

		if (request.method == "GET") {
			handle_get_request(client_fd, request, matched_location);
		} else if (request.method == "POST") {
			handle_post_request(client_fd, request, matched_location);
		} else if (request.method == "DELETE") {
			handle_post_request(client_fd, request, matched_location);
		} else {
			send_error_response(client_fd, 405, "Method Not Allowed");
		}


	} catch (const std::exception& e) {
		send_error_response(client_fd, 500, "Internal Server Error");
	}

	close(client_fd);

}

void TestServer::handle_get_request(int client_fd, const HttpRequest& request, const std::string& location)
{
	try {
		if(!is_method_allowed("GET", location)) {
			send_error_response(client_fd, 405, "Method Not Allowed");
			return;
		}

		std::string file_path = server_config.root + request.path;

		struct stat path_stat;
		if (stat(file_path.c_str(), &path_stat) == 0) {
			if (S_ISDIR(path_stat.st_mode)) {
				for (size_t i = 0; i < server_config.index_files.size(); i++) {
					std::string index_path = file_path + "/" + server_config.index_files[i];
					if (access(index_path.c_str(), F_OK) == 0) {
						send_file_response(client_fd, index_path);
						return;
					}
				}

				bool autoindex = false;
				for (size_t i = 0; i < server_config.locations.size(); i++) {
					if (server_config.locations[i].path == location) {
						autoindex = server_config.locations[i].auto_index;
						break;
					}
				}
				if (autoindex) {
					send_directory_listing(client_fd, file_path, request.path);
	
				} else {
					send_error_response(client_fd, 403, "Forbidden");
				}
			} else {
				send_file_response(client_fd, file_path);
			}
		} else {
			send_error_response(client_fd, 404, "Not Found");
		}
	} catch (const std::exception& e) {
		send_error_response(client_fd, 500, "Internal Server Error");
	}
}

void TestServer::send_directory_listing(int client_fd, const std::string& directory_path, const std::string& request_path)
{
	DIR* dir = opendir(directory_path.c_str());
	if (!dir) {
		send_error_response(client_fd, 403, "Forbidden");
		return;
	}

	std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html><head><meta charset=\"utf-8\">"
         << "<title>Index of " << request_path << "</title>"
         << "</head><body>"
         << "<h1>Index of " << request_path << "</h1>"
		 << "<hr><ul>";

	if (request_path != "/") {
		std::string parent = request_path;
		if (!parent.empty() && parent.back() == "/")
			parent.push_back();
		size_t slash = parent.find_last_of('/');
		parent = slash == std::string::npos ? "/" : parent.substr(0, slash + 1);
		html << "<li><a href=/" << parent << "\">..</a></li>";
	}

	for (struct dirent* ent = readdir(dir); ent != NULL; ent = readdir(dir)) 
	{
		std::string name = ent->d_name;
		if (name == "." || name == "..")
			continue;
		std::string href = request_path;
		if (href.empty() || href[0] != '/')
			href = "/" + href;
		if (!href.empty() && href.back() != '/')
			href += "/";
		href += name;

		bool is_dir = false;
		if (ent->d_type == DT_DIR) {
			is_dir = true;
		} else if (ent->d_type == DT_UNKNOWN) {
			std::string full = directory_path;
			if (!full.empty() && full.back() != '/')
				full += name;

			struct stat st;
			if (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) 
				is_dir = true;
		}
		
		html << "<li><a href=\"" << href;
        if (is_dir) html << "/";
        html << "\">" << name;
        if (is_dir) html << "/";
        html << "</a></li>";
	}
	closedir(dir);

	html << "</ul><hr></body></html>";

	std::string body = html.str();

	std::ostringstream resp;
	resp << "HTTP/1.1 200 OK\r\n"
		 << "Content-Type: text/html\r\n"
		 << "Content-Length: " << body.length() << "\r\n"
		 << "Connection: close\r\n"
		 << "\r\n"
		 << body;
	
	std::string response = resp.str();
	write(client_fd, response.c_str(), response.length());
}


void TestServer::handle_post_request(int client_fd, const HttpRequest& request, const std::string& location)
{
	try {
		if (!is_method_allowed("POST", location)) {
			send_error_response(client_fd, 405, "Method Not Allowed");
			return;
		}

        std::string response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "Content-Length: 23\r\n\r\n"
                              "<h1>POST Received!</h1>";		
		write(client_fd, response.c_str(), response.length());						
	} catch (const std::exception& e) {
		send_error_response(client_fd, 500, "Internal Server Error");
	}
}

void TestServer::handle_delete_request(int client_fd, const HttpRequest& request, const std::string& location)
{
	try {
		if (!is_method_allowed("DELETE", location)) {
			send_error_response(client_fd, 405, "Method Not Allowed");
			return;
		}

		if (request.path.empty() || request.path == "/") {
			send_error_response(client_fd, 400, "Bad Request");
			return;
		}

		if (has_path_traversal(request.path)) {
			send_error_response(client_fd, 400, "Bad Request");
			return;
		}

		std::string file_path = server_config.root + request.path;

		struct stat path_stat;

		if (stat(file_path.c_str(), &path_stat) != 0 ) {
			send_error_response(client_fd, 404, "Not Found");
			return;
		}

		if (S_ISDIR(path_stat.st_mode)) {
			send_error_response(client_fd, 409, "Conflict");
			return;
		}

		if (unlink(file_path.c_str()) != 0) {
			if (errno == EACCES || errno == EPERM) {
				send_error_response(client_fd, 403, "Forbidden");
			} else {
				send_error_response(client_fd, 500, "Internal Server Error");
			}
			return;
		}

		std::string respond= 
		            "HTTP/1.1 204 No Content\r\n"
        			"Content-Length: 0\r\n"
            		"Connection: close\r\n"
            		"\r\n";
		write(client_fd, respond.c_str(), respond.length());
	} catch (const std::exception& e) {
		send_error_response(client_fd, 500, "Internal Server Error");
	}
}

std::string TestServer::find_matching_location(const std::string& path)
{
	std::string best_match = "/";
	size_t best_match_length = 0;
	bool found_root = false;

	for (size_t i = 0; i < server_config.locations.size(); i++) {
		const Location& loc = server_config.locations[i];

		if (path.find(loc.path) == 0) {
			bool is_exact_match = (path.length() == loc.path.length());
			bool is_subpath_match = (loc.path == "/" || (path.length() > loc.path.length() && path[loc.path.length()] == '/'));
			if (is_exact_match || is_subpath_match) {
				if (loc.path.length() > best_match_length) {
					best_match = loc.path;
					best_match_length = loc.path.length();
				}
				if (loc.path == "/") {
					found_root = true;
				}
			}
		}
	}
	if (server_config.locations.empty() || (!found_root && best_match_length == 0)) {
		best_match = "/";
	}
	return best_match;
}
// Tambah temporary vector
/*
	pfds.fd = -1;
*/
void TestServer::launch()
{
	if (pfds.empty())
	{
		pollfd server_pfd;
		server_pfd.fd = get_socket()->get_sock();
		server_pfd.events = POLLIN;
		pfds.push_back(server_pfd);
	}
	while (true)
	{
		int ready = poll(pfds.data(), pfds.size(), -1);
		if (ready < 0) {
			perror("Poll error");
			continue;
		}
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
					pfds.erase(pfds.begin() + i);
				}
			}
		}
	}
}

static bool has_path_travelsal(const std::string& p)
{
	return (p.find("..") != std::string::npos);
}

void TestServer::print_config() const
{
    std::cout << "\n=============== SERVER CONFIGURATION ===============" << std::endl;
    
    // Print Server info
    std::cout << "Server Name: " << server_config.server_name << std::endl;
    std::cout << "Port: " << server_config.port << std::endl;
    std::cout << "Root: " << server_config.root << std::endl;
    
    // Print Index Files
    std::cout << "Index Files: ";
    if (server_config.index_files.empty()) {
        std::cout << "None";
    } else {
        for (size_t i = 0; i < server_config.index_files.size(); ++i) {
            std::cout << server_config.index_files[i];
            if (i < server_config.index_files.size() - 1)
                std::cout << ", ";
        }
    }
    std::cout << std::endl;
    
    // Print Error Pages
    std::cout << "Error Pages: ";
    if (server_config.error_pages.empty()) {
        std::cout << "None" << std::endl;
    } else {
        std::cout << std::endl;
        for (std::map<int, std::string>::const_iterator it = server_config.error_pages.begin(); 
             it != server_config.error_pages.end(); ++it) {
            std::cout << "  " << it->first << " -> " << it->second << std::endl;
        }
    }
    
    // Print Locations
    std::cout << "Locations (" << server_config.locations.size() << "):" << std::endl;
    if (server_config.locations.empty()) {
        std::cout << "  No locations configured." << std::endl;
    } else {
        for (size_t i = 0; i < server_config.locations.size(); ++i) {
            const Location& loc = server_config.locations[i];
            
            std::cout << "\n  --- Location " << (i + 1) << " ---" << std::endl;
            std::cout << "  Path: " << loc.path << std::endl;
            std::cout << "  Methods: ";
            
            // Print allowed methods
            std::vector<std::string> methods;
            if (loc.allow_get) methods.push_back("GET");
            if (loc.allow_post) methods.push_back("POST");
            if (loc.allow_delete) methods.push_back("DELETE");
            
            if (methods.empty()) {
                std::cout << "None";
            } else {
                for (size_t j = 0; j < methods.size(); ++j) {
                    std::cout << methods[j];
                    if (j < methods.size() - 1)
                        std::cout << ", ";
                }
            }
            std::cout << std::endl;
            
            std::cout << "  CGI Path: " << (loc.cgi_path.empty() ? "Not set" : loc.cgi_path) << std::endl;
			std::cout << "  Upload Path: " << (loc.upload_path.empty() ? "Not set" : loc.upload_path) << std::endl;
            std::cout << "  Auto Index: " << (loc.auto_index ? "On" : "Off") << std::endl;
            
            std::cout << "  CGI Extensions: ";
            if (loc.cgi_ext.empty()) {
                std::cout << "None";
            } else {
                for (size_t j = 0; j < loc.cgi_ext.size(); ++j) {
                    std::cout << loc.cgi_ext[j];
                    if (j < loc.cgi_ext.size() - 1)
                        std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "=================================================" << std::endl;
}