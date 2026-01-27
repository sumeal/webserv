/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/26 13:09:10 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Core.h"
#include "CGI_data.h"
#include <csignal>
#include <cstddef>
#include <poll.h>
#include "CgiExecute.h"
#include "Client.h"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "Respond.h"
// #include <cstdint>
// #include <stdexcept>
// #include <set>
// #include <chrono> //havetodelete
// #include "CgiRequest.h"

Core::Core() : _needCleanup(false)
{}

Core::~Core() 
{
	// std::set<CgiExecute*> deletedPtrs;
	
	// for (std::map<int, CgiExecute*>::iterator it = _cgiMap.begin(); it != _cgiMap.end(); it++)
	// {
	// 	CgiExecute* current = it->second;
	// 	if (current != NULL && deletedPtrs.find(current) == deletedPtrs.end())
	// 	{
	// 		deletedPtrs.insert(current);
	// 		delete(current);
	// 	}
	// }
	// _cgiMap.clear();//redundant as this only being used when we clear vector but keep the heap object. now the heap object already gone no use
}

void	Core::run()
{
	int			clientCount = 0;
	
	int loopCount = 0;
	while (1)
	{

		int result = poll(&_fds[0], _fds.size(), 4000);
		if (result < 0) {
			perror("Poll error");
			continue;
		}
		//handleTimeout(); //better put before poll so poll dont run timeout client
		for (long unsigned int i = 0; i < _fds.size(); i++)
		{
			int	oriFd = _fds[i].fd;
			if (oriFd == -1)
				continue ;
			int	revents = _fds[i].revents;
			if (!revents)
				continue;
			try
			{
				if (oriFd == server_fd) {
					if (revents & POLLIN)
					{
						accepter();
						continue ;
					}
				}
				else {
					
					Client* client = _clients[oriFd];

					if (!client) {
						std::cout << "No client found for FD " << oriFd << std::endl;
						continue;
					}
					// Only for client socket (not cgi socket)
					if ((revents & POLLHUP) && oriFd == client->getSocket())
					{
						deleteClient(client); //handle disconnect;
						i--;
						continue ;
					}
					if (revents & POLLIN || revents & POLLHUP)  //POLLHUP for CGI
					{
						// 24jan26.is it that every pollin will go in here. what if the pollin is for cgi? shouldnt it better to check the state first then implement this part? so if the state is cgi dont need to go through here?
						if (client->state == READ_REQUEST) //mad added
						{ //mad added
							std::string raw_request = client->readRawRequest();
							if (!raw_request.empty())
								parse_http_request(client, raw_request);
							else
								client->state = DISCONNECTED;
						} //mad added
						client->procInput(i, _fds[i]);
						if (_clients.find(oriFd) == _clients.end()) 
						{
                        	i--;
                        	continue;
						}
					}
					if (_fds[i].revents & POLLOUT || revents & POLLHUP)
						client->procOutput(i, _fds[i]);
					//clean from maps/fd registration queue
					handleTransition(client);
				}
			}
			catch (int statusCode)
			{
				Client* client = _clients[oriFd];
				if (client) {
					handleClientError(client, statusCode);
				}
			}
			if (_fds[i].fd == -1)
			{
				_clients.erase(oriFd);
				_needCleanup = true;
			}
		}
		//delete all in the delete list
		fdCleanup();
		addStagedFds(); //handle all sementara
		if(clientCount >= 10 && _clients.empty())
			break ;
		loopCount++;
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
	{
		deleteClient(client);
	}
}
void	Core::handleTimeout()
{
	time_t now = time(NULL);
    std::vector<int> clients_to_delete; // Store FDs to delete
    //chatgpt
    // First pass: identify clients to delete
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client* client = it->second;
        
        if (client && client->isIdle(now))
        {
            if (client->state == READ_REQUEST || client->state == FINISHED)
            {
                clients_to_delete.push_back(it->first);
            }
            else
            {
                handleClientError(client, client->GetCgiExec() && client->GetCgiExec()->getpid() ? 504 : 408);
            }
        }
    }
    
    // Second pass: delete clients
    for (size_t i = 0; i < clients_to_delete.size(); i++)
    {
        int fd = clients_to_delete[i];
        Client* client = _clients[fd];
        if (client) {
            deleteClient(client);
        }
    }
}


//comment jap ada error
// void	Core::launchCgi(Client* client, t_location& locate, t_request& request)
// {
// 	client->GetCgiExec()->preExecute();
// 	client->GetCgiExec()->execute();
// 	//the part im trying to implement
// 	cgiRegister(client); //in core because it change the struct that hold all the list
	
// }

void	Core::cgiRegister(Client* client)
{
	CgiExecute* CgiExec = client->GetCgiExec();
	int ToCgi = CgiExec->getpipeToCgi();
	int FromCgi = CgiExec->getpipeFromCgi();
	
	// _cgiMap[ToCgi] = CgiExec;
	// _cgiMap[FromCgi] = CgiExec;
	_clients[ToCgi] = client;
	_clients[FromCgi] = client;
	struct pollfd pfdRead;
	pfdRead.fd = FromCgi;
	pfdRead.events = POLLIN;
	pfdRead.revents = 0;
	_stagedFds.push_back(pfdRead);
	struct pollfd pfdWrite;
	pfdWrite.fd = ToCgi;
	pfdWrite.events = POLLOUT;
	pfdWrite.revents = 0;
	_stagedFds.push_back(pfdWrite);
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
			//do i need to set revent to 0?
			return;
		}
	}
}

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
	
	if (client->hasCgi()) //supposely dont need since all have closed their CGI
	{
		int	pipeFromCgi = client->GetCgiExec()->getpipeFromCgi();
		int	pipeToCgi	= client->GetCgiExec()->getpipeToCgi();
		fdPreCleanup(pipeFromCgi, 0);
		fdPreCleanup(pipeToCgi, 0);
		// _clients.erase(pipeFromCgi);
		// _clients.erase(pipeToCgi);
	}
	std::cout << "Client " << socketFd << " deleted from poll" << std::endl;
	_clients.erase(socketFd);
	fdPreCleanup(socketFd, 0);
	delete client;
}

void	Core::removeFd(int fd)
{
	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
	{
		if (it->fd == fd)
		{
			close(it->fd);
			_fds.erase(it);
			_clients.erase(fd);
			// _cgiMap.erase(fd);
			fd = -1;
			return ;
		}
	}
}

void	Core::fdPreCleanup(int fd, int i)
{
	if (i)
	{
		int fd2 = _fds[i].fd;
		if (fd2 == -1)
			return ;
		close(fd2);
		_clients.erase(fd2);
		// _cgiMap.erase(fd2);
		_fds[i].fd = -1;
		_needCleanup = true;
		return;
	}
	else 
	{	
		for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
		{
			if (it->fd == fd)
			{
				close(it->fd);
				_clients.erase(fd);
				// _cgiMap.erase(fd);
				it->fd = -1;
				_needCleanup = true;
				return ;
			}
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
	//How do Muzz store in map for the custom error file
	// std::string errorPath = error_pages[statusCode]; //will do map later
	std::string errorPath = "Tiputiputipu"; //will do map later
	//Generate.
	if (!errorPath.empty())
		client->getRespond().findErrorBody(errorPath);
	client->getRespond().buildErrorResponse(statusCode);
	//Cleanup for finished problematic CGI
	if (client->hasCgi())
	{
		client->GetCgiExec()->clearCgi();
		fdPreCleanup(client->GetCgiExec()->getpipeFromCgi(), 0);
		fdPreCleanup(client->GetCgiExec()->getpipeToCgi(), 0);
	}
	//State update
	client->state = SEND_RESPONSE;
	respondRegister(client);
	client->state = WAIT_RESPONSE;
	//Poll update
	// for (size_t i = 0; i < _fds.size(); i++)
	// {
	// 	if (_fds[i].fd == client->getSocket())
	// 	{
	// 		_fds[i].events = POLLIN | POLLOUT;
	// 		_fds[i].revents = 0;
	// 		break ;
	// 	}
	// }
}

void	Core::addStagedFds()
{
	if (_stagedFds.empty())
		return ;
	_fds.insert(_fds.end(), _stagedFds.begin(), _stagedFds.end());
	_stagedFds.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Another way of remove instead of iterating loop
// struct IsInvalid 
//{
//     // This is the "logic" remove_if uses to decide
//     bool operator()(const struct pollfd& p) const {
//         return (p.fd == -1); 
//     }
// };
//
// void Core::fdCleanup() {
//     // 1. Mark and move
//     std::vector<struct pollfd>::iterator new_end = std::remove_if(
//         _fds.begin(), 
//         _fds.end(), 
//         IsInvalid() // This creates a temporary instance of your rule
//     );
//     // 2. Actually resize
//     _fds.erase(new_end, _fds.end());
// }
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Simplified way but maybe loss slow of preCleanup
// void	Core::fdPreCleanup(int fd)
// {
// 	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
// 	{
// 		if (it->fd == fd)
// 		{
// 			close(it->fd);
// 			_clients.erase(fd);
// 			_cgiMap.erase(fd);
// 			it->fd = -1;
// 			_needCleanup = true;
// 			return ;
// 		}
// 	}
// 	// Safety: If for some reason the FD wasn't in the poll vector, 
//     // we still need to close it and clean the maps to avoid leaks.
// 	close(fd);
// 	_clients.erase(fd);
// 	_cgiMap.erase(fd);
// }
///////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// // Testing only
// void Core::acceptMockConnections(t_location& locate, t_request& request, int& clientCount)
// {
//     // Only create new clients if we are under our test limit
//     if (clientCount < 2) 
//     {
//         int dummyFd = clientCount + 200;
//         Client* client = new Client();
        
//         // Setup CGI (simulation of a CGI request)
//         client->setCgiExec(new CgiExecute(client, request, locate));
        
//         // clientRegister should add the FD to your vector 
//         // and set _fds[i].events = POLLIN
//         clientRegister(dummyFd, client); 
        
//         clientCount++;
//     }
// }

// //Testing
// void Core::forceMockEvents()
// {
//     for (size_t i = 0; i < _fds.size(); i++)
//     {
//         // We look up the client associated with this specific FD
//         Client* c = _clients[_fds[i].fd];

        // If the client exists and is waiting to be read, we 'fake' 
        // the activity that the OS would normally detect.
//         if (c && c->state == READ_REQUEST && !c->revived) 
//         {
//             _fds[i].revents = POLLIN; 
//         }
//     }
// }

void	Core::pathCheck(std::string path)
{
	std::string rootPath = "/home/user/42/webserv/www";
	
	size_t start	= 1;
	size_t end		= rootPath.find("/", 1);
	std::vector<std::string>	rootPathVec;
	
	while (end != std::string::npos)
	{
		std::string name = rootPath.substr(start, end - start);
		if (!name.empty())
		{
			if (name == "..")
				rootPathVec.pop_back();
			else if (name != ".")
				rootPathVec.push_back(name);
		}
		start = end + 1;
		end = rootPath.find("/", start);
	}
	std::string name = rootPath.substr(start);
	if (!name.empty())
		rootPathVec.push_back(name);

	size_t	startPath	= 0;
	size_t	endPath		= path.find("/");
	size_t	rootFloor	= rootPathVec.size();
	while(endPath != std::string::npos)
	{
		std::string name2 = path.substr(startPath, endPath - startPath); 
		if (!name2.empty() && name2 != ".")
		{
			if (name2 == "..")
			{
				if (rootPathVec.empty())
					throw("error");
				else if (rootPathVec.size() > rootFloor)
					rootPathVec.pop_back();
			}
			else
				rootPathVec.push_back(name2);
		}
		startPath = endPath + 1;
		endPath = path.find("/", startPath); 
	}
	std::string name2 = path.substr(startPath);
	if (!name2.empty() && name2 != "." && name2 != "..")
		rootPathVec.push_back(name2);
	else if (name2 == "..")
		rootPathVec.pop_back();
	std::string fullPath = "/";
	for (size_t i = 0; i < rootPathVec.size(); i++)
	{
		fullPath = fullPath + rootPathVec[i];
		if (i < rootPathVec.size() - 1)
			fullPath = fullPath + "/";
	}
	struct stat fileInfo;
	if (stat(fullPath.c_str(), &fileInfo) != 0)
		throw (403);
	if(S_ISDIR(fileInfo.st_mode))
	{
		//check default file usually defined in config
		std::string defaultPath = fullPath;
		if (!defaultPath.empty() && defaultPath[defaultPath.length() - 1] != '/')
        	defaultPath += "/";
		defaultPath += "index.html"; //commonly index.html
		struct stat defaultInfo;
		//if custom file  not there send error
		if (stat(defaultPath.c_str(), &defaultInfo) != 0)
			throw (403);
		else
		{
			if (access(defaultPath.c_str(), R_OK) != 0)
				throw (403);
			fullPath = defaultPath;
		}
	}
	else if(S_ISREG(fileInfo.st_mode))
	{
		if (access(fullPath.c_str(), F_OK) != 0)
			throw (404);
		if (access(fullPath.c_str(), R_OK) != 0)
			throw (403);		
	}
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

// void	Core::run( t_location& locate, t_request& request)
// {
// 	int			clientCount = 0;
// 	std::vector<Client*> activeClients;
	
// 	int loopCount = 0;
// 	while (1)
// 	{
// 		acceptMockConnections(locate, request, clientCount);
// 		int result = poll(&_fds[0], _fds.size(), 4000);
// 		forceMockEvents();
// 		//					ACTUAL LOOP
// 		for (int i = 0; i < _fds.size(); i++)
// 		{
// 			int	*fd = &_fds[i].fd;
// 			if (*fd == -1)
// 				continue ;
// 			int	revents = _fds[i].revents;
// 			Client* client = _clients[*fd];
// 			int	pipeFromCgi = -1;
// 			int pipeToCgi = -1;
// 			if (client->GetCgiExec())
// 			{
// 				pipeFromCgi = client->GetCgiExec()->getpipeFromCgi();
// 				pipeToCgi = client->GetCgiExec()->getpipeToCgi();
// 			}
// 			// if (!revents)
// 			// 	continue;
// 			try
// 			{
// 				if (revents & POLLIN || revents & POLLHUP)
// 				{
// 					if (client->state == READ_REQUEST)
// 					{
// 						//parser/config part.used to read http
// 						// if (/*read request finished*/)
// 						if (!client->GetCgiExec()->isCGI())
// 						{
// 							client->getRespond().procNormalOutput();
// 							client->getRespond().buildResponse();
// 							respondRegister(client);
// 							client->state = SEND_RESPONSE;
// 						}
// 						else
// 							client->state = EXECUTE_CGI;
// 						//if execute CGI state
// 						if (client->state == EXECUTE_CGI)
// 						{
// 							if (!client->GetCgiExec()->isCGI())
// 							{
// 								client->state = SEND_RESPONSE; //need to ask muzz
// 							}
// 							launchCgi(client, locate, request);
// 							client->state = WAIT_CGI;
// 						}
// 					}
// 					if (client->state == WAIT_CGI && *fd == pipeFromCgi)
// 					{
// 						client->GetCgiExec()->readExec();
// 						client->GetCgiExec()->cgiState();
// 						if (client->GetCgiExec()->isReadDone())
// 						{
// 							//removeFd(pipeFromCgi);
// 							fdPreCleanup(pipeFromCgi, i);
// 							if (client->GetCgiExec()->isDone())
// 							{
// 								client->getRespond().procCgiOutput(client->GetCgiExec()->getOutput());
// 								client->getRespond().buildResponse();
// 								respondRegister(client);
// 								client->state = SEND_RESPONSE;
// 							}
// 						}
// 					}
// 				}
// 				if (_fds[i].revents & POLLOUT || revents & POLLHUP)
// 				{
// 					if (client->state == WAIT_CGI && *fd == pipeToCgi)
// 					{
// 						client->GetCgiExec()->writeExec();
// 						client->GetCgiExec()->cgiState();
// 						if (client->GetCgiExec()->isWriteDone())
// 						{
// 							fdPreCleanup(pipeToCgi, i);
// 							if (client->GetCgiExec()->isDone())
// 							{
// 								client->getRespond().procCgiOutput(client->GetCgiExec()->getOutput());
// 								client->getRespond().buildResponse();
// 								respondRegister(client);
// 								client->state = SEND_RESPONSE;
// 							}
// 						}
// 					}
// 					if (client->state == SEND_RESPONSE /*&& *fd == client->getSocket()*/)
// 					{
// 						int status = client->getRespond().sendResponse();
// 						if (status)
// 						{
// 							client->getRespond().printResponse();
// 							client->state = FINISHED;
// 						}
// 					}
// 				}
// 			}
// 			catch (int statusCode)
// 			{
// 				handleClientError(client, statusCode);
// 			}
// 			if (client->state == SEND_RESPONSE /*&& *fd == client->getSocket()*/)
// 			{
// 				int status = client->getRespond().sendResponse();
// 				if (status)
// 				{
// 					client->getRespond().printResponse();
// 					client->state = FINISHED;
// 				}
// 			}
// 			if (client->state == FINISHED) // timeout only need to trigger this
// 			{
// 				//clear/delete/break
// 				deleteClient(client);
// 			}
// 		}
// 		//delete all in the delete list
// 		fdCleanup();
// 		addStagedFds();
// 		if(clientCount >= 10 && _clients.empty())
// 			break ;
// 	}
// }

//muzz part real

void Core::parse_config(const std::string &filename)
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
				temp_location = Parse::location_init(line);
			}
			current_state = (ParseState)new_state;
        }
        else if (current_state == LOCATION) {
            int new_state = Parse::parse_location(line, current_state, temp_location);
            if (new_state == SERVER && current_state == LOCATION) {
				if (temp_location.root.empty()) {
					temp_location.root = server_config.root;
				}
                server_config.locations.push_back(temp_location);
            }
            current_state = (ParseState)new_state;
        }
    }

    file.close();
    std::cout << "Config parsing completed" << std::endl;
}

void Core::parse_http_request(Client* current_client, const std::string raw_req)
{
	if (!current_client) {
		std::cerr << "Error: null client pointer in parse_http_request" <<std::endl;
		return;
	}
	s_HttpRequest& current_request = current_client->getRequest();
	std::istringstream request_stream(raw_req);
	std::string line;

	if (std::getline(request_stream, line)) {
		std::istringstream req_line(line);
		req_line >> current_request.method >> current_request.uri >> current_request.http_version;

		/*
		uri = "/cgi-bin/hello.py?name=John&age=25"

		query_pos = 18  // Position of '?'

		path = "/cgi-bin/hello.py"     // Everything before '?'
		query = "name=John&age=25"     // Everything after '?'
		*/
	
		size_t query_pos = current_request.uri.find('?');
		if (query_pos != std::string::npos) {
			current_request.path = current_request.uri.substr(0, query_pos);
			current_request.query = current_request.uri.substr(query_pos +1);
		}
		else {
			current_request.path = current_request.uri;
		}
	}

	// while std::getline return something and doesnot end with "\r" and is not empty
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
	
    parseRequestBody(current_client, request_stream, raw_req);

    parseConnectionHeader(current_client, current_request);
	
    std::string path = current_request.path;
    
    if (path.find("/cgi-bin/") == 0 || 
        path.find(".py") != std::string::npos ||
        path.find(".pl") != std::string::npos ||
        path.find(".php") != std::string::npos) {
        
        current_client->setHasCgi(true);
        std::cout << "CGI request: " << path << std::endl;
    } else {
        current_client->setHasCgi(false);
        std::cout << "Normal file request: " << path << std::endl;
    }
	
	putIntoCached(current_request);
	std::cout << "Parsed: " << current_request.method << " " << current_request.path << std::endl;
	debugHttpRequest(current_request);
	current_client->state = HANDLE_REQUEST;
}

void Core::initialize_server()
{
	// Create listening socket with config port
	server_fd = SocketUtils::create_listening_socket(server_config.port);
	if (server_fd < 0) {
		std::cerr << "Failed to create server socket" << std::endl;
		return;
	}
	if (SocketUtils::set_non_blocking(server_fd) < 0) {
		perror("Failed to set non-blocking for Listening Socket");
		return;
	}
	// Add the listening socket to the fd
	serverRegister(server_fd);
	std::cout << "Server initialized with:" << std::endl;
	std::cout << "Port: " << server_config.port << std::endl;
}

void Core::accepter()
{
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);

	new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
	std::cout << "New Connection " << new_socket << " Accepted" <<std::endl;
	if (new_socket < 0) {
		perror("Accept fail");
		return;
	}
	
	if (SocketUtils::set_non_blocking(new_socket) < 0) {
		perror("Failed to set non-blocking");
		return;
	}
	
	Client* new_client = new Client(server_config);
	clientRegister(new_socket, new_client);
	
	std::cout << "Client registered seccessfully !" << std::endl;
}


void Core::clientRegister(int clientFD, Client* client)
{
	_clients[clientFD] = client;
	client->setSocket(clientFD);
	client->state = READ_REQUEST;

	// Configure the Respond object with server settings
	client->getRespond().setServerName(server_config.server_name);
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
    
    if (server_config.locations.empty()) {
        std::cout << "❌ No locations found in configuration" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < server_config.locations.size(); i++) {
        print_location_config(server_config.locations[i], i + 1);
        std::cout << std::endl;
    }
    
    std::cout << "📊 Total locations parsed: " << server_config.locations.size() << std::endl;
}

void Core::parseConnectionHeader(Client* client, const s_HttpRequest& request)
{
    bool keepAlive = false;
    
    if (request.http_version == "HTTP/1.1") {
        keepAlive = true;
    } else {
        keepAlive = false;
    }
    
    std::map<std::string, std::string>::const_iterator conn_it = request.headers.find("Connection");
    if (conn_it == request.headers.end()) {
        conn_it = request.headers.find("connection");
    }
    
    if (conn_it != request.headers.end()) {
        std::string conn_value = conn_it->second;
        
        for (size_t i = 0; i < conn_value.length(); i++) {
            conn_value[i] = std::tolower(conn_value[i]);
        }
        
        if (conn_value.find("keep-alive") != std::string::npos) {
            keepAlive = true;
            std::cout << "🔄 Keep-Alive requested by client" << std::endl;
        } else if (conn_value.find("close") != std::string::npos) {
            keepAlive = false;
            std::cout << "❌ Connection close requested by client" << std::endl;
        }
    }
    
    if (keepAlive) {
        client->setConnStatus(KEEP_ALIVE);
        std::cout << "✅ Connection will be kept alive (" << request.http_version << ")" << std::endl;
    } else {
        client->setConnStatus(CLOSE);
    }
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
        if (request.body.length() <= 100) {
            std::cout << "\"" << request.body << "\"" << std::endl;
        } else {
            std::cout << "\"" << request.body.substr(0, 150) << "...\" (truncated)" << std::endl;
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

void Core::putIntoCached(s_HttpRequest& request)
{
    std::cout << "🔄 Caching parsed headers..." << std::endl;
    
    // 1. Cache Host header and extract port
    std::map<std::string, std::string>::iterator host_it = request.headers.find("Host");
    if (host_it != request.headers.end()) {
        std::string host_header = host_it->second;
        
        // Check if port is specified in Host header (e.g., "localhost:8088")
        size_t port_pos = host_header.find(':');
        if (port_pos != std::string::npos) {
            request.host = host_header.substr(0, port_pos);
            std::string port_str = host_header.substr(port_pos + 1);
            request.port = atoi(port_str.c_str());
        } else {
            request.host = host_header;
            // Default port based on protocol (HTTP = 80, HTTPS = 443)
            request.port = 80;  // Default for HTTP
        }
        std::cout << "   ✅ Host: \"" << request.host << ":" << request.port << "\"" << std::endl;
    } else {
        request.host = "";
        request.port = 0;
        std::cout << "   ⚠️ No Host header found" << std::endl;
    }
    
    // 2. Cache Content-Length header
    std::map<std::string, std::string>::iterator cl_it = request.headers.find("Content-Length");
    if (cl_it != request.headers.end()) {
        request.content_length = static_cast<size_t>(atoi(cl_it->second.c_str()));
        std::cout << "   ✅ Content-Length: " << request.content_length << " bytes" << std::endl;
    } else {
        request.content_length = 0;
        if (request.method == "POST" || request.method == "PUT" || request.method == "PATCH") {
            std::cout << "   ⚠️ No Content-Length for " << request.method << " request" << std::endl;
        }
    }
    
    // 3. Cache Content-Type header
    std::map<std::string, std::string>::iterator ct_it = request.headers.find("Content-Type");
    if (ct_it != request.headers.end()) {
        request.content_type = ct_it->second;
        std::cout << "   ✅ Content-Type: \"" << request.content_type << "\"" << std::endl;
    } else {
        request.content_type = "";
        if (request.method == "POST" || request.method == "PUT" || request.method == "PATCH") {
            std::cout << "   ⚠️ No Content-Type for " << request.method << " request" << std::endl;
        }
    }
    
    // 4. Cache Keep-Alive status (set by parseConnectionHeader)
    request.keep_alive = (request.http_version == "HTTP/1.1");  // Default
    
    std::map<std::string, std::string>::iterator conn_it = request.headers.find("Connection");
    if (conn_it == request.headers.end()) {
        conn_it = request.headers.find("connection");
    }
    
    if (conn_it != request.headers.end()) {
        std::string conn_value = conn_it->second;
        // Convert to lowercase
        for (size_t i = 0; i < conn_value.length(); i++) {
            conn_value[i] = std::tolower(conn_value[i]);
        }
        
        if (conn_value.find("keep-alive") != std::string::npos) {
            request.keep_alive = true;
        } else if (conn_value.find("close") != std::string::npos) {
            request.keep_alive = false;
        }
    }
    std::cout << "   ✅ Keep-Alive: " << (request.keep_alive ? "TRUE" : "FALSE") << std::endl;
    
    std::cout << "✅ Header caching complete" << std::endl;
}
void Core::parseRequestBody(Client* client, std::istringstream& request_stream, const std::string& raw_req)
{
    s_HttpRequest& request = client->getRequest();
    
    // Get Content-Length if it exists
    size_t content_length = 0;
    std::map<std::string, std::string>::iterator cl_it = request.headers.find("Content-Length");
    if (cl_it != request.headers.end()) {
        content_length = static_cast<size_t>(atoi(cl_it->second.c_str()));
    }
    
    std::cout << "🔍 Body parsing - Content-Length: " << content_length << std::endl;
    
    if (content_length == 0) {
        std::cout << "   ✅ No body expected (Content-Length: 0)" << std::endl;
        return;
    }
    
    // Method 1: Parse remaining content from stream
    std::string body_content;
    std::string body_line;
    
    while (std::getline(request_stream, body_line)) {
        body_content += body_line;
        if (!request_stream.eof()) {
            body_content += "\n";
        }
    }
    
    // Method 2: Alternative - find body in raw request
    if (body_content.empty()) {
        std::cout << "   🔄 Stream method failed, using raw request method..." << std::endl;
        
        // Find the end of headers (double CRLF)
        size_t body_start = raw_req.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            body_start += 4; // Skip past "\r\n\r\n"
            body_content = raw_req.substr(body_start);
        } else {
            // Try single LF variant
            body_start = raw_req.find("\n\n");
            if (body_start != std::string::npos) {
                body_start += 2; // Skip past "\n\n"
                body_content = raw_req.substr(body_start);
            }
        }
    }
    
    // Validate body length
    if (body_content.length() != content_length) {
        std::cout << "   ⚠️ Body length mismatch - Expected: " << content_length 
                  << ", Got: " << body_content.length() << std::endl;
        
        // Truncate if too long, or use what we have if too short
        if (body_content.length() > content_length) {
            body_content = body_content.substr(0, content_length);
            std::cout << "   🔧 Truncated body to expected length" << std::endl;
        }
    }
    
    request.body = body_content;
    
    std::cout << "   ✅ Body parsed successfully - " << request.body.length() << " bytes" << std::endl;
    
    // Debug body content (first 100 chars)
    if (!request.body.empty()) {
        std::string preview = request.body;
        if (preview.length() > 100) {
            preview = preview.substr(0, 100) + "...";
        }
        std::cout << "   📄 Body preview: \"" << preview << "\"" << std::endl;
    }
}