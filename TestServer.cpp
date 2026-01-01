#include "TestServer.hpp"
#include "Parse.hpp"

TestServer::TestServer() : SimpleServer(AF_INET, SOCK_STREAM, 0 ,8080, INADDR_ANY, 10)
{
	memset(buffer, 0, sizeof(buffer));
	
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
    
    print_config();
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