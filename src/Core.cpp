/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/13 15:12:45 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../inc/Core.h"
#include "./../inc/CGI_data.h"
#include "./../inc/CgiExecute.h"
#include "./../inc/Client.h"
#include "./../inc/Respond.h"
#include <csignal>
#include <cstddef>
#include <poll.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>

extern volatile sig_atomic_t g_shutdown;

Core::Core() : _needCleanup(false)
{}

Core::~Core() 
{}

void	Core::run()
{
	while (g_shutdown == 0)
	{
		handleTimeout();

		int result = poll(&_fds[0], _fds.size(), 10000);
		if (result < 0) 
		{
			if (errno == EINTR)
				return ;
			perror("Polls error");
			continue;
		}
		if (result > 0)
			handleEvents();
		fdCleanup();
		addStagedFds();
	}
}

void	Core::handleEvents()
{
	for (long unsigned int i = 0; i < _fds.size(); i++)
	{
		int	fd = _fds[i].fd;
		if (fd == -1)
			continue ;
		int	revents = _fds[i].revents;
		if (!revents)
			continue;
		try
		{
			if (_serverFd.find(fd) != _serverFd.end()) 
			{
				if (revents & POLLIN)
				{
					size_t server_index = _serverFd[fd];
					accepter(fd, server_index);
					continue ;
				}
			}
			else 
			{
				Client* client = _clients[fd];
				if (!client) 
				{
					std::cout << "No client found for FD " << fd << std::endl;
					continue;
				}
				if ((revents & POLLHUP) && fd == client->getSocket()) //client disconnect
				{
					deleteClient(client);
					continue ;
				}
				if (revents & POLLIN || revents & POLLHUP)  //POLLHUP for CGI
				{
					handleRequestRead(client);
					client->procInput(_fds[i]);
				}
				if (_fds[i].revents & POLLOUT || revents & POLLHUP)
					client->procOutput(_fds[i]);
				//clean from maps/fd registration queue
				handleTransition(client);
			}
		}
		catch (int statusCode)
		{
			Client* client = _clients[fd];
			if (client) 
				handleClientError(client, statusCode);
		}
		if (_fds[i].fd == -1)
		{
			_clients.erase(fd);
			_needCleanup = true;
		}
	}
}

void	Core::handleRequestRead(Client* client)
{
	if (client->state != READ_REQUEST)
		return ;
	bool request_ready = client->readHttpRequest();
	if (client->isDisconnected())
		client->state = DISCONNECTED;
	else if (request_ready && client->isRequestComplete()) 
	{
		std::string raw_request = client->getCompleteRequest();
		if (!raw_request.empty())
		{
			parse_http_request(client, raw_request);
			client->resetRequestBuffer();
		}
		else
			client->state = DISCONNECTED;
	}
}

void	Core::handleTransition(Client* client)
{
	if (client->state == EXECUTE_CGI)
	{
		cgiRegister(client);
		client->state = WAIT_CGI;
	}
	else if (client->state == SEND_RESPONSE)
	{
		respondRegister(client); //set POLLOUT
		client->state = WAIT_RESPONSE;
	}
	// Note: Removed the immediate WAIT_RESPONSE check - response will be sent when POLLOUT triggers
	else if (client->state == FINISHED) // timeout only need to trigger this
	{
		if (!client->isKeepAlive())
		{
			deleteClient(client);
			return ;
		}
		else
		{
			client->resetClient();
			client->state = READ_REQUEST;
			int clientFd = client->getSocket();
            for (size_t i = 0; i < _fds.size(); i++)
            {
                if (_fds[i].fd == clientFd)
                {
                    _fds[i].events = POLLIN;
                    _fds[i].revents = 0;
                    break;
                }
            }
			return ;
		}
	}
	else if (client->state == DISCONNECTED)
		deleteClient(client);
}

void	Core::handleTimeout()
{
	time_t now = time(NULL);
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client* client = it->second;
        
        if (client && client->state == 3 && client->isIdle(now))
			handleClientError(client, client->GetCgiExec() && client->GetCgiExec()->getpid() ? 504 : 408);
    }
}

void	Core::cgiRegister(Client* client)
{
	CgiExecute* CgiExec = client->GetCgiExec();
	int ToCgi = CgiExec->getpipeToCgi();
	int FromCgi = CgiExec->getpipeFromCgi();
	
	client->setLastActivity();
	_clients[ToCgi] = client;
	_clients[FromCgi] = client;
	struct pollfd pfdWrite;
	pfdWrite.fd = ToCgi;
	pfdWrite.events = POLLOUT;
	pfdWrite.revents = 0;
	_stagedFds.push_back(pfdWrite);
	struct pollfd pfdRead;
	pfdRead.fd = FromCgi;
	pfdRead.events = POLLIN;
	pfdRead.revents = 0;
	_stagedFds.push_back(pfdRead);
}

void	Core::respondRegister(Client* client)
{
	int	ClientFd = client->getSocket();

	for (long unsigned int i = 0; i < _fds.size(); i++)
	{
		int VecFd = _fds[i].fd;
		if (VecFd == ClientFd)
		{
			_fds[i].events = POLLOUT;
			return;
		}
	}
}

/*
	POLLIN → Notify me when I can read (data available)
	POLLOUT → Notify me when I can write (buffer space available)
*/

void Core::serverRegister(int serverFd)
{
    struct pollfd pfd;
    pfd.fd = serverFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);
}

void	Core::deleteClient(Client* client)
{
	int	socketFd = client->getSocket();
	
	if (client->getHasCgi()) //extra check
	{
		int	pipeFromCgi = client->GetCgiExec()->getpipeFromCgi();
		int	pipeToCgi	= client->GetCgiExec()->getpipeToCgi();
		fdPreCleanup(pipeFromCgi);
		fdPreCleanup(pipeToCgi);
	}
	_clients.erase(socketFd);
	fdPreCleanup(socketFd);
	delete client;
}

void	Core::fdPreCleanup(int fd)
{
	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
	{
		if (it->fd == fd)
		{
			close(it->fd);
			_clients.erase(fd);
			it->fd = -1;
			_needCleanup = true;
			return ;
		}
	}	
}

void	Core::fdCleanup()
{
	if (!_needCleanup)
		return ;
	for (size_t i = 0; i < _fds.size(); )
	{
		if (_fds[i].fd == -1)
			_fds.erase(_fds.begin() + i);
		else
			i++;
	}
	_needCleanup = 0;
}

void	Core::handleClientError(Client* client, int statusCode)
{
	std::string errorPath = client->getServerConfig().error_pages[statusCode];
	//Generate.
	if (!errorPath.empty())
		client->getRespond().findErrorBody(errorPath);
	client->getRespond().buildErrorResponse(statusCode);
	//Cleanup for finished problematic CGI
	if (client->isCgiExecuted())
	{
		client->GetCgiExec()->clearCgi();
		fdPreCleanup(client->GetCgiExec()->getpipeFromCgi());
		fdPreCleanup(client->GetCgiExec()->getpipeToCgi());
	}
	//State update
	client->state = SEND_RESPONSE;
	respondRegister(client);
	client->state = WAIT_RESPONSE;
}

void	Core::addStagedFds()
{
	if (_stagedFds.empty())
		return ;
	_fds.insert(_fds.end(), _stagedFds.begin(), _stagedFds.end());
	_stagedFds.clear();
}

void Core::parse_config(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open()) 
	{
        std::cerr << "Error: config file failed" << std::endl;
        return;
    }
	_usedPorts.clear();
    ParseState current_state = OUTSIDE;
    std::string line;
	
    while (std::getline(file, line)) 
	{
        line = Parse::trim_line(line);
        
        if (Parse::is_comment_or_empty(line))
            continue;
        
        if (current_state == OUTSIDE) 
		{
            current_state = (ParseState)Parse::parse_outside(line, current_state);
			if (current_state == SERVER) 
			{
				temp_server = t_server();
				temp_server.client_max_body_size = 1048576;
			}
        }
        else if (current_state == SERVER) 
		{
            int new_state = Parse::parse_server(line, current_state, temp_server, _usedPorts);
			if (new_state == LOCATION && current_state == SERVER)
				temp_location = Parse::location_init(line);
			else if (new_state == OUTSIDE && current_state == SERVER)
				server_config.push_back(temp_server);
			current_state = (ParseState)new_state;
        }
        else if (current_state == LOCATION)
		{
            int new_state = Parse::parse_location(line, current_state, temp_location);
            if (new_state == SERVER && current_state == LOCATION)
			{
				if (temp_location.root.empty())
					temp_location.root = temp_server.root;
                temp_server.locations.push_back(temp_location);
            }
            current_state = (ParseState)new_state;
        }
    }
    file.close();
}

void Core::parse_http_request(Client* current_client, const std::string raw_req)
{
	if (!current_client) 
	{
		std::cerr << "Error: null client pointer in parse_http_request" <<std::endl;
		return;
	}
	s_HttpRequest& current_request = current_client->getRequest();
	std::istringstream request_stream(raw_req);
	std::string line;
	current_request.is_cgi = false;
	if (std::getline(request_stream, line)) 
	{
		std::istringstream req_line(line);
		req_line >> current_request.method >> current_request.uri >> current_request.http_version;

		/*
		uri = "/cgi-bin/hello.py?name=John&age=25"

		query_pos = 18  // Position of '?'

		path = "/cgi-bin/hello.py"     // Everything before '?'
		query = "name=John&age=25"     // Everything after '?'
		*/
	
		size_t query_pos = current_request.uri.find('?');
		if (query_pos != std::string::npos) 
		{
			current_request.path = current_request.uri.substr(0, query_pos);
			current_request.query = current_request.uri.substr(query_pos +1);
		}
		else
			current_request.path = current_request.uri;
	}

	// while std::getline return something and does not end with "\r" and is not empty
	while (std::getline(request_stream, line) && line != "\r" && !line.empty()) 
	{
		size_t colon_pos = line.find(':');
		if (colon_pos != std::string::npos) 
		{
			std::string header_name = line.substr(0, colon_pos);
			std::string header_value = line.substr(colon_pos + 2);

			if (!header_value.empty() && header_value[header_value.length() - 1] == '\r')
				header_value.erase(header_value.length() - 1);
			current_request.headers[header_name] = header_value;
			if (header_name == "Cookie")
				current_request.cookie = header_value;
		}
	}
	
	size_t body_start = raw_req.find("\r\n\r\n");
	if (body_start == std::string::npos) 
	{
		body_start = raw_req.find("\n\n");
		if (body_start != std::string::npos)
			body_start += 2;
	} 
	else
		body_start += 4;
	
	if (body_start != std::string::npos && body_start < raw_req.length()) 
		current_request.body = raw_req.substr(body_start);
	else
		current_request.body = "";

    parseConnectionHeader(current_client, current_request);
	
    std::string path = current_request.path;
    
    if (path.find("/cgi-bin/") == 0 || 
        path.find(".py") != std::string::npos ||
        path.find(".php") != std::string::npos) 
	{
        	current_client->setHasCgi(true);
        	current_request.is_cgi = true;  // Set the flag in request too
	} 
	else 
	{
        current_client->setHasCgi(false);
        current_request.is_cgi = false; // Set to false for non-CGI
    }
	
	// debugHttpRequest(current_request); //importantdebug
	current_client->checkBestLocation();
	current_client->state = HANDLE_REQUEST;
}

void Core::initialize_server()
{
	std::cout << std::endl;
	for(size_t i = 0; i < server_config.size(); i++) 
	{
		int server_fd = SocketUtils::create_listening_socket(server_config[i].port);
		if (server_fd < 0)
			exit(1);
		if (SocketUtils::set_non_blocking(server_fd) < 0) 
		{
			close(server_fd);
			continue;
		}
		_serverFd[server_fd] = i;
		serverRegister(server_fd);
	}
}

void Core::accepter(int server_fd, size_t server_index)
{
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);

	int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
	
	if (new_socket < 0) 
	{
		perror("Accept fail");
		return;
	}
	
	if (SocketUtils::set_non_blocking(new_socket) < 0) 
	{
		perror("Failed to set non-blocking");
		return;
	}
	
	Client* new_client = new Client(server_config[server_index], _cookies);
	clientRegister(new_socket, new_client, server_index);
	
}


void Core::clientRegister(int clientFD, Client* client, size_t server_index)
{
	_clients[clientFD] = client;
	client->setSocket(clientFD);
	client->state = READ_REQUEST;

	// Configure the Respond object with server settings
	client->getRespond().setServerName(server_config[server_index].server_name);
	client->getRespond().setSocketFd(clientFD);

	struct pollfd pfd;
	pfd.fd = clientFD;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_stagedFds.push_back(pfd);
}


void Core::print_location_config(const t_location& location, int index)
{
    std::cout << "=== Location Block " << index << " ===" << std::endl;
    std::cout << "Path: " << location.path << std::endl;
    std::cout << "Root: " << (location.root.empty() ? "[not set]" : location.root) << std::endl;
    std::cout << "CGI Enabled: " << (location.cgi_enabled ? "YES" : "NO") << std::endl;
	std::cout << "Has Redirect: " << (location.has_redirect ? "YES" : "NO") << std::endl;
    
    if (location.cgi_enabled) {
        std::cout << "CGI Path: " << (location.interp.empty() ? "[not set]" : location.interp) << std::endl;
        std::cout << "CGI Extensions: ";
        if (location.cgi_extension.empty()) {
            std::cout << "[none]";
        } else {
            for (size_t i = 0; i < location.cgi_extension.size(); i++) {
                std::cout << location.cgi_extension[i];
                if (i < location.cgi_extension.size() - 1) std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << "HTTP Methods: ";
    if (!location.allow_get && !location.allow_post && !location.allow_delete) {
        std::cout << "[none allowed]";
    } else {
        if (location.allow_get) std::cout << "GET ";
        if (location.allow_post) std::cout << "POST ";
        if (location.allow_delete) std::cout << "DELETE ";
    }
    std::cout << std::endl;
    
    std::cout << "Auto Index: " << (location.auto_index ? "ON" : "OFF") << std::endl;
    std::cout << "=========================" << std::endl;
}

void Core::print_all_locations()
{
    std::cout << std::endl << "📍 PARSED LOCATION CONFIGURATION:" << std::endl;
    
	for (size_t i = 0; i < server_config.size(); i++) {
	t_server server_configuration = server_config[i];
    if (server_configuration.locations.empty()) {
        std::cout << "❌ No locations found in configuration" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < server_configuration.locations.size(); i++) {
        print_location_config(server_configuration.locations[i], i + 1);
        std::cout << std::endl;
    }
    
    std::cout << "📊 Total locations parsed: " << server_configuration.locations.size() << std::endl << std::endl;
	}
}

void Core::parseConnectionHeader(Client* client, const s_HttpRequest& request)
{
    bool keepAlive = false;
    
    if (request.http_version == "HTTP/1.1")
        keepAlive = true;
    else
        keepAlive = false;
    
    std::map<std::string, std::string>::const_iterator conn_it = request.headers.find("Connection");
    if (conn_it == request.headers.end())
        conn_it = request.headers.find("connection");
    
    if (conn_it != request.headers.end()) 
	{
        std::string conn_value = conn_it->second;
        
        for (size_t i = 0; i < conn_value.length(); i++)
            conn_value[i] = std::tolower(conn_value[i]);
        
        if (conn_value.find("keep-alive") != std::string::npos)
            keepAlive = true;
        else if (conn_value.find("close") != std::string::npos)
            keepAlive = false;
    }
    
    if (keepAlive)
        client->setConnStatus(KEEP_ALIVE);
    else
        client->setConnStatus(CLOSE);
}

void Core::debugHttpRequest(const t_HttpRequest& request)
{
	std::cout << "\n" << std::string(60, '=') << std::endl;
	std::cout << "🔍 HTTP REQUEST DEBUG INFORMATION" << std::endl;
	std::cout << std::string(60, '=') << std::endl;

	// Request Line Information
	std::cout << "📋 REQUEST LINE:" << std::endl;
	std::cout << "   Method:       \"" << request.method << "\"" << std::endl;
	std::cout << "   URI:          \"" << request.uri << "\"" << std::endl;
	std::cout << "   Path:         \"" << request.path << "\"" << std::endl;
	std::cout << "   Query:        \"" << request.query << "\"" << std::endl;
	std::cout << "   HTTP Version: \"" << request.http_version << "\"" << std::endl;

	std::cout << "\n📦 HEADERS (" << request.headers.size() << " total):" << std::endl;
	if (request.headers.empty()) {
		std::cout << "   (No headers found)" << std::endl;
	} else {
		for (std::map<std::string, std::string>::const_iterator it = request.headers.begin();
             it != request.headers.end(); ++it) {
            std::cout << "   " << std::setw(20) << std::left << (it->first + ":") 
                      << " \"" << it->second << "\"" << std::endl;
        }
    }

    std::cout << "\n🎯 PARSED HEADER CACHE:" << std::endl;
    std::cout << "   Host:           \"" << request.host << "\"" << std::endl;
    std::cout << "   Port:           " << request.port << std::endl;
    std::cout << "   Content-Length: " << request.content_length << std::endl;
    std::cout << "   Content-Type:   \"" << request.content_type << "\"" << std::endl;
    std::cout << "   Keep-Alive:     " << (request.keep_alive ? "TRUE" : "FALSE") << std::endl;
    
    std::cout << "\n📄 BODY INFORMATION:" << std::endl;
    std::cout << "   Body Size:      " << request.body.length() << " bytes" << std::endl;
    if (request.body.empty()) {
        std::cout << "   Body Content:   (empty)" << std::endl;
    } else {
        std::cout << "   Body Preview:   ";
        if (request.body.length() <= 200) {
            std::cout << "\"" << request.body << "\"" << std::endl;
        } else {
            std::cout << "\"" << request.body.substr(0, 200) << "...\" (truncated)" << std::endl;
        }
        
        // Show full body if it looks like chunked encoding
        if (request.body.find('\n') != std::string::npos && 
            request.body.find("0\n") != std::string::npos) {
            std::cout << "🔍 Full chunked body:" << std::endl;
            std::cout << "\"" << request.body << "\"" << std::endl;
        }
        
        // Show body in hex if it contains binary data
        bool has_binary = false;
        for (size_t i = 0; i < request.body.length() && i < 50; i++) {
            if (request.body[i] < 32 && request.body[i] != '\n' && request.body[i] != '\r' && request.body[i] != '\t') {
                has_binary = true;
                break;
            }
        }
		
		if (has_binary) {
			std::cout << "   Body (Hex):     ";
			for (size_t i = 0; i < request.body.length() && i < 32; i++) {
				printf("%02x ", (unsigned char)request.body[i]);
			}
			if (request.body.length() > 32) std::cout << "...";
			std::cout << std::endl;
		}
	}

	std::cout << "\n🚩 STATE FLAGS:" << std::endl;
	std::cout << "   Is CGI:         " << (request.is_cgi ? "TRUE" : "FALSE") << std::endl;

    std::cout << std::string(60, '=') << std::endl;
    std::cout << "🔍 END HTTP REQUEST DEBUG" << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;
}

void	Core::CleanupAll()
{
	std::map<int, Client*>::iterator it;
	it = _clients.begin();
	while (it != _clients.end())
	{
		Client* client = it->second;
		_clients.erase(it++);
		
		if (client)
			deleteClient(client);
	}
}
