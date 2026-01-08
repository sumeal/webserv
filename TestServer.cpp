#include "TestServer.hpp"
#include "Parse.hpp"

TestServer::TestServer()
{	
	// Initialize socket to inv// ===================================================================

	server_fd = -1;
	new_socket = -1;
	
	// Initialize config values to defaults
	server_config.port = 8080;  // Default port
	server_config.server_name = "localhost";
	server_config.root = "./www";
	
	// Initialize temp_location
	temp_location.path = "";
	temp_location.allow_get = false;
	temp_location.allow_post = false;
	temp_location.allow_delete = false;
	temp_location.cgi_path = "";
	temp_location.auto_index = false;
	
	memset(buffer, 0, sizeof(buffer));
}

TestServer::~TestServer()
{
	// Clean up socket
	if (server_fd >= 0) {
		SocketUtils::close_socket(server_fd);
	}
}

void TestServer::initialize_server()
{
	// Create listening socket with config port
	server_fd = SocketUtils::create_listening_socket(server_config.port);
	if (server_fd < 0) {
		std::cerr << "Failed to create server socket" << std::endl;
		return;
	}
	
	std::cout << "Server initialized with:" << std::endl;
	std::cout << "Port: " << server_config.port << std::endl;
	std::cout << "Server Name: " << server_config.server_name << std::endl;
	std::cout << "Root: " << server_config.root << std::endl;
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
        line = Parse::trim_line(line);
        
        if (Parse::is_comment_or_empty(line))
            continue;
        
        if (current_state == OUTSIDE) {
            current_state = (ParseState)Parse::parse_outside(line, current_state);
        }
        else if (current_state == SERVER) {
            int new_state = Parse::parse_server(line, current_state, server_config);
            
            if (new_state == LOCATION && current_state == SERVER) {
                std::vector<std::string> tokens = Parse::split_line(line);
                if (tokens.size() >= 2 && tokens[0] == "location") {
                    temp_location = Location();
                    temp_location.path = tokens[1];
                    temp_location.allow_get = false;
                    temp_location.allow_post = false;
                    temp_location.allow_delete = false;
                    temp_location.auto_index = false;
                }
            }
            current_state = (ParseState)new_state;
        }
        else if (current_state == LOCATION) {
            int new_state = Parse::parse_location(line, current_state, temp_location);
            
            if (new_state == SERVER && current_state == LOCATION) {
                server_config.locations.push_back(temp_location);
            }
            current_state = (ParseState)new_state;
        }
    }
    
    file.close();
    std::cout << "Config parsing completed" << std::endl;
}

void TestServer::handler(int client_fd)
{
	char buffer[42000];
	memset(buffer, 0, sizeof(buffer));
	ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
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
				header_value.erase(header_value.length() - 1);
			}
			current_request.headers[header_name] = header_value;
		}
	}
	
	// // Add simple debug output
	// std::cout << "Parsed: " << current_request.method << " " << current_request.path << std::endl;

	print_parsed_request(current_request, client_fd);

}


void TestServer::print_parsed_request(const HttpRequest& request, int client_fd) const
{
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    HTTP REQUEST PARSED                      ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    
    // Client Information
    std::cout << "║ Client FD: " << std::setw(49) << std::left << client_fd << "║" << std::endl;
    
    // Request Line Information
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║                      REQUEST LINE                           ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ Method:        " << std::setw(45) << std::left << request.method << "║" << std::endl;
    std::cout << "║ URI:           " << std::setw(45) << std::left << request.uri << "║" << std::endl;
    std::cout << "║ Path:          " << std::setw(45) << std::left << request.path << "║" << std::endl;
    std::cout << "║ Query String:  " << std::setw(45) << std::left << (request.query.empty() ? "(none)" : request.query) << "║" << std::endl;
    std::cout << "║ HTTP Version:  " << std::setw(45) << std::left << request.http_version << "║" << std::endl;
    
    // Headers Information
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║                         HEADERS                             ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    
    if (request.headers.empty()) {
        std::cout << "║ No headers found                                            ║" << std::endl;
    } else {
        for (std::map<std::string, std::string>::const_iterator it = request.headers.begin(); 
             it != request.headers.end(); ++it) {
            std::string header_line = it->first + ": " + it->second;
            if (header_line.length() > 58) {
                header_line = header_line.substr(0, 55) + "...";
            }
            std::cout << "║ " << std::setw(59) << std::left << header_line << "║" << std::endl;
        }
    }
    
    // Body Information  
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║                           BODY                              ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    
    if (request.body.empty()) {
        std::cout << "║ No body content                                             ║" << std::endl;
    } else {
        std::cout << "║ Body Length: " << std::setw(47) << std::left << request.body.length() << "║" << std::endl;
        
        // Show first few lines of body (truncated for display)
        std::istringstream body_stream(request.body);
        std::string body_line;
        int line_count = 0;
        
        while (std::getline(body_stream, body_line) && line_count < 3) {
            if (body_line.length() > 58) {
                body_line = body_line.substr(0, 55) + "...";
            }
            std::cout << "║ " << std::setw(59) << std::left << body_line << "║" << std::endl;
            line_count++;
        }
        
        if (line_count >= 3 && !body_stream.eof()) {
            std::cout << "║ " << std::setw(59) << std::left << "(body truncated...)" << "║" << std::endl;
        }
    }
    
    // Request Analysis
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║                      ANALYSIS                               ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    
    // Determine content type
    std::string content_type = "Unknown";
    if (request.path.find(".html") != std::string::npos) content_type = "HTML";
    else if (request.path.find(".css") != std::string::npos) content_type = "CSS";
    else if (request.path.find(".js") != std::string::npos) content_type = "JavaScript";
    else if (request.path.find(".png") != std::string::npos) content_type = "PNG Image";
    else if (request.path.find(".jpg") != std::string::npos) content_type = "JPEG Image";
    else if (request.path.find(".ico") != std::string::npos) content_type = "Icon";
    else if (request.path.find(".py") != std::string::npos) content_type = "Python CGI";
    else if (request.path == "/") content_type = "Root/Index";
    
    std::cout << "║ File Type:     " << std::setw(45) << std::left << content_type << "║" << std::endl;
    std::cout << "║ Header Count:  " << std::setw(45) << std::left << request.headers.size() << "║" << std::endl;
    
    // Check for specific headers
    bool has_host = request.headers.find("Host") != request.headers.end();
    bool has_user_agent = request.headers.find("User-Agent") != request.headers.end();
    bool has_content_length = request.headers.find("Content-Length") != request.headers.end();
    
    std::cout << "║ Has Host:      " << std::setw(45) << std::left << (has_host ? "Yes" : "No") << "║" << std::endl;
    std::cout << "║ Has User-Agent:" << std::setw(45) << std::left << (has_user_agent ? "Yes" : "No") << "║" << std::endl;
    std::cout << "║ Has Content-Len:" << std::setw(44) << std::left << (has_content_length ? "Yes" : "No") << "║" << std::endl;
    
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl << std::endl;
}

// ===================================================================
// PLACEHOLDER: RESPONDER (FOR TEAMMATE INTEGRATION)
// ===================================================================
void TestServer::responder(int client_fd)
{
	// TODO: Your teammate will implement response handling here
	// This should handle GET, POST, DELETE requests
	// For now, just send a simple response and close connection
	
	std::string simple_response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello World!\n";
	write(client_fd, simple_response.c_str(), simple_response.length());
	close(client_fd);
}

// ===================================================================
// PLACEHOLDER: RESPONDER (FOR TEAMMATE INTEGRATION)  
// ===================================================================
// void TestServer::responder(int client_fd)
// {
//     // Get the parsed request for this client
//     HttpRequest& request = client_req[client_fd];
    
//     // Build the file path
//     std::string file_path = server_config.root;
//     if (request.path == "/") {
//         file_path += "/index.html";  // Default to index.html for root
//     } else {
//         file_path += request.path;
//     }
    
//     // Try to open and read the file
//     std::ifstream file(file_path.c_str(), std::ios::binary);
//     if (file.is_open()) {
//         // Get file size
//         file.seekg(0, std::ios::end);
//         size_t file_size = file.tellg();
//         file.seekg(0, std::ios::beg);
        
//         // Read file content
//         std::string file_content;
//         file_content.resize(file_size);
//         file.read(&file_content[0], file_size);
//         file.close();
        
//         // Send HTTP response with file content
//         std::ostringstream response;
//         response << "HTTP/1.1 200 OK\r\n";
//         response << "Content-Type: text/html\r\n";
//         response << "Content-Length: " << file_size << "\r\n";
//         response << "Connection: close\r\n";
//         response << "\r\n";
//         response << file_content;
        
//         std::string response_str = response.str();
//         write(client_fd, response_str.c_str(), response_str.length());
        
//         std::cout << "Served file: " << file_path << " (" << file_size << " bytes)" << std::endl;
//     } else {
//         // File not found - send 404 error
//         std::string not_found = 
//             "HTTP/1.1 404 Not Found\r\n"
//             "Content-Type: text/html\r\n"
//             "Content-Length: 47\r\n"
//             "Connection: close\r\n"
//             "\r\n"
//             "<html><body><h1>404 Not Found</h1></body></html>";
        
//         write(client_fd, not_found.c_str(), not_found.length());
//         std::cout << "File not found: " << file_path << std::endl;
//     }
    
//     close(client_fd);
// }

void TestServer::launch()
{
	if (server_fd < 0) {
		std::cerr << "Error: Server not initialized. Call initialize_server() first." << std::endl;
		return;
	}
	
	// Setup initial poll structure with our server socket
	if (pfds.empty())
	{
		pollfd server_pfd;
		server_pfd.fd = server_fd;  // Use our simple server socket
		server_pfd.events = POLLIN;
		pfds.push_back(server_pfd);
	}
	
	std::cout << "Server launched and listening..." << std::endl;
	std::cout << "Max connections supported: ~1000 (limited by poll and system resources)" << std::endl;
	
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
				if (pfds[i].fd == server_fd)  // New connection on our server socket
				{
					// Basic connection limit check
					if (pfds.size() >= 1000) {
						std::cerr << "Warning: Max connections reached, rejecting new connections" << std::endl;
						continue;
					}
					
					accepter();
					if (new_socket > 0) {  // Only add if accept succeeded
						pollfd new_pfd;
						new_pfd.fd = new_socket;
						new_pfd.events = POLLIN;
						pfds.push_back(new_pfd);
					}
				}
				else
				{
					handler(pfds[i].fd);
					pfds.erase(pfds.begin() + i);
					i--;
				}
			}
		}
	}
}

// ===================================================================
// ACCEPTOR (NEW CLIENT CONNECTION HANDLING)
// ===================================================================
void TestServer::accepter()
{
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);

	new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
	std::cout << "New Connection " << new_socket - 3 << " Accepted" <<std::endl;
	if (new_socket < 0) {
		perror("Accept fail");
		return;
	}
	
	if (SocketUtils::set_non_blocking(new_socket) < 0) {
		perror("Failed to set non-blocking");
		return;
	}
}

// ===================================================================
// UTILITY FUNCTION FOR CONFIG DEBUG (OPTIONAL)
// ===================================================================
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