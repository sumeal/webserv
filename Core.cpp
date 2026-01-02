/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/02 15:56:09 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Core.h"
#include "CGI_data.h"
#include <cstdint>
#include <poll.h>
#include <stdexcept>
#include "CgiExecute.h"
#include "Client.h"
#include <iostream>
#include <set>
#include <unistd.h>
#include <chrono> //havetodelete
// #include "CgiRequest.h"

Core::Core() 
{}

Core::~Core() 
{
	std::set<CgiExecute*> deletedPtrs;
	
	for (std::map<int, CgiExecute*>::iterator it = _cgiMap.begin(); it != _cgiMap.end(); it++)
	{
		CgiExecute* current = it->second;
		if (current != NULL && deletedPtrs.find(current) == deletedPtrs.end())
		{
			deletedPtrs.insert(current);
			delete(current);
		}
	}
	_cgiMap.clear();//redundant as this only being used when we clear vector but keep the heap object. now the heap object already gone no use
}

void	Core::run( t_location& locate, t_request& request)
{
	int			clientCount = 0;
	std::vector<Client*> activeClients;
	
	int loopCount = 0;
	while (1)
	{
		acceptMockConnections(locate, request, clientCount);
		int result = poll(&_fds[0], _fds.size(), 4000);
		forceMockEvents();
		//					ACTUAL LOOP
		for (int i = 0; i < _fds.size(); i++)
		{
			int	revents = _fds[i].revents;
			int	*fd = &_fds[i].fd;
			Client* client = _clients[*fd];
			int	pipeFromCgi = client->GetCgiExec()->getpipeFromCgi();
			int	pipeToCgi = client->GetCgiExec()->getpipeToCgi();

			// if (!revents)
			// 	continue;
			try
			{
				if (revents & POLLIN || revents & POLLHUP)
				{
					if (client->state == READ_REQUEST)
					{
						//parser/config part.used to read http
						// if (/*read request finished*/)
						if (!client->GetCgiExec()->isCGI())
						{
							client->getRespond().procNormalOutput();
							client->getRespond().buildResponse();
							respondRegister(client);
							client->state = SEND_RESPONSE;
						}
						else
							client->state = EXECUTE_CGI;
						//if execute CGI state
						if (client->state == EXECUTE_CGI)
						{
							if (!client->GetCgiExec()->isCGI())
								client->state = SEND_RESPONSE; //need to ask muzz
							launchCgi(client, locate, request);
							client->state = WAIT_CGI;
							// break ; //added
						}
					}
					if (client->state == WAIT_CGI && *fd == pipeFromCgi)
					{
						client->GetCgiExec()->readExec();
						client->GetCgiExec()->cgiState();
						if (client->GetCgiExec()->isReadDone())
						{
							//removeFd(pipeFromCgi);
							fdPreCleanup(pipeFromCgi, i);
							if (client->GetCgiExec()->isDone())
							{
								client->getRespond().procCgiOutput(client->GetCgiExec()->getOutput());
								client->getRespond().buildResponse();
								respondRegister(client);
								client->state = SEND_RESPONSE;
							}
							// break ; //may not need to break
						}
					}
				}
				if (_fds[i].revents & POLLOUT)
				{
					if (client->state == WAIT_CGI && *fd == pipeToCgi)
					{
						client->GetCgiExec()->writeExec();
						client->GetCgiExec()->cgiState();
						if (client->GetCgiExec()->isWriteDone())
						{
							// removeFd(pipeToCgi);
							fdPreCleanup(pipeToCgi, i);
							if (client->GetCgiExec()->isDone())
							{
								client->getRespond().procCgiOutput(client->GetCgiExec()->getOutput());
								client->getRespond().buildResponse();
								respondRegister(client);
								client->state = SEND_RESPONSE;
							}
							// break; //may not need to break
						}
					}
					if (client->state == SEND_RESPONSE && *fd == client->getSocket())
					{
						int status = client->getRespond().sendResponse();
						if (status)
						{
							client->getRespond().printResponse();
							client->state = FINISHED;
						}
					}
				}
			}
			catch (int statusCode)
			{
				handleClientError(client, statusCode, i);
			}
			if (client->state == FINISHED)
				std::cout << "clientstate:" << client->state << std::endl; //debug
			if (client->state == FINISHED) // timeout only need to trigger this
			{
				//clear/delete/break
				deleteClient(client);
				break ;
			}
		}
		//delete all in the delete list
		fdCleanup();
		addStagedFds();
		if(clientCount >= 10 && _clients.empty())
			break ;
	}
}

void	Core::launchCgi(Client* client, t_location& locate, t_request& request)
{
	client->GetCgiExec()->preExecute();
	client->GetCgiExec()->execute();
	//the part im trying to implement
	cgiRegister(client); //in core because it change the struct that hold all the list
	
}

void	Core::cgiRegister(Client* client)
{
	CgiExecute* CgiExec = client->GetCgiExec();
	int ToCgi = CgiExec->getpipeToCgi();
	int FromCgi = CgiExec->getpipeFromCgi();
	
	_cgiMap[ToCgi] = CgiExec;
	_cgiMap[FromCgi] = CgiExec;
	_clients[ToCgi] = client;
	_clients[FromCgi] = client;
	struct pollfd pfdRead;
	pfdRead.fd = FromCgi;
	pfdRead.events = POLLIN;
	pfdRead.revents = 0;
	// _fds.push_back(pfdRead);
	_stagedFds.push_back(pfdRead);
	struct pollfd pfdWrite;
	pfdWrite.fd = ToCgi;
	pfdWrite.events = POLLOUT;
	pfdWrite.revents = 0;
	// _fds.push_back(pfdWrite);
	_stagedFds.push_back(pfdWrite);
}

void	Core::respondRegister(Client* client)
{
	int	ClientFd = client->getSocket();

	for (int i = 0; i < _fds.size(); i++)
	{
		int VecFd = _fds[i].fd;
		if (VecFd == ClientFd)
		{
			_fds[i].events = POLLOUT;
			return;
		}
	}
}

//Geminied socket listening
void Core::serverRegister(int serverFd) //GPTed  muzz part
{
    struct pollfd pfd;
    pfd.fd = serverFd;
    pfd.events = POLLIN; // Listen for new incoming connections
    pfd.revents = 0;
    _fds.push_back(pfd);
    
    // Note: You don't usually need a map entry for the listener 
    // unless you have multiple ports/servers.
}

//Geminied Client Socket
void Core::clientRegister(int clientFd, Client* client)
{
    _clients[clientFd] = client; // Map the socket to the Client object
	client->setSocket(clientFd);
    struct pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN; // Listen for HTTP request data
    pfd.revents = 0;
    _fds.push_back(pfd);
}

void	Core::deleteClient(Client* client)
{
	removeFd(client->GetCgiExec()->getpipeFromCgi()); //just extra check
	removeFd(client->GetCgiExec()->getpipeToCgi()); //just extra check
	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
	{
		if (it->fd == client->getSocket())
		{
			_fds.erase(it);
			_clients.erase(client->getSocket());
			client->setSocket(-1);
			break ;
		}
	}
	//removeFd(client->getSocket());
	//removefd client socket
	//update server capacity?
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
			_cgiMap.erase(fd);
			fd = -1;
			return ;
		}
	}
	close(fd);
	_clients.erase(fd);
	fd = -1;
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
		_cgiMap.erase(fd2);
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
				_cgiMap.erase(fd);
				it->fd = -1;
				_needCleanup = true;
				return ;
			}
		}
	}
}

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

void	Core::handleClientError(Client* client, int statusCode, int index)
{
	//How do Muzz store in map for the custom error file
	// std::string errorPath = error_pages[statusCode]; //will do map later
	std::string errorPath = "Tiputiputipu"; //will do map later
	//Generate.
	if (!errorPath.empty())
		client->getRespond().findErrorBody(errorPath);
	client->getRespond().buildErrorResponse(statusCode);
	//Cleanup for CGI
	if (client->isCgiOn())
	{
		fdPreCleanup(client->GetCgiExec()->getpipeFromCgi(), 0);
		fdPreCleanup(client->GetCgiExec()->getpipeToCgi(), 0);
		client->GetCgiExec()->clearCgi();
	}
	//State update
	client->state = SEND_RESPONSE;
	//Poll update
	for (size_t i = 0; i < _fds.size(); i++)
	{
		if (_fds[i].fd == client->getSocket())
		{
			_fds[i].events = POLLOUT;
			_fds[i].revents = 0;
			break ;
		}
	}
}

void	Core::addStagedFds()
{
	if (_stagedFds.empty())
		return ;
	_fds.insert(_fds.end(), _stagedFds.begin(), _stagedFds.end());
	_stagedFds.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

// void	Core::run( t_location& locate, t_request& request)
// {
// 	CgiRequest	requestor(request, locate);
// 	Client*		client = new Client(&requestor);
// 	CgiExecute	executor(client, request, locate);

// 	auto start = std::chrono::high_resolution_clock::now(); //have todelete
// 	int loopCount = 0;
// 	while (1)
// 	{
// 		std::cout << "looping" << std::endl;
// 		// usleep(1000); //debug
// 		//if request state
// 		if(client->state == READ_REQUEST)
// 		{
// 			//parser/config part
// 			client->state = EXECUTE_CGI;
// 		}
// 		//if execute CGI state
// 		// std::cout << "state1: " << client->state << std::endl; //debug
// 		// usleep(1000); //debug
// 		if (client->state == EXECUTE_CGI)
// 		{
// 			if (!requestor.isCGI())
// 				throw(std::runtime_error("CGI: request not CGI"));
// 			launchCgi(executor, locate, request);
// 			client->state = WAIT_CGI;
// 		}
// 		// std::cout << "state2: " << client->state << std::endl; //debug
// 		// usleep(1000); //debug
// 		int result = poll(&_fds[0], _fds.size(), 4000);
// 		if (result > 0 && client->state == WAIT_CGI)
// 		{
// 			// std::cout << "insideornot" << std::endl; //debug
// 			cgiWait(executor);
// 		}
// 		//if send output to client
// 		if (client->state == SEND_RESPONSE)
// 		{
// 			//for now
// 			std::cout << "output: " << executor.getOutput() << std::endl;
// 			break; 
// 		}		
// 		if (client->state == FINISHED)
// 		{
// 			//clear/delete/break
// 			// std::cout << "output: " << executor.getOutput() << std::endl;
// 			delete(client);
// 			client = NULL;
// 			break; 
// 		}
// 		// std::cout << "inside6" << std::endl;//debug
// 		// usleep(1000); //debug
// 		std::cout << "Loop ran: " << loopCount++ << " times." << std::endl; //debug
// 	}
// 	auto end = std::chrono::high_resolution_clock::now(); //debug
// 	std::chrono::duration<double, std::milli> elapsed = end - start; //debug
// 	std::cout << "Total Time: " << elapsed.count() << " ms" << std::endl; //debug
// }

// for (std::map<int, t_CGI *>::iterator it = _cgi_map.begin(); 
// 	it != _cgi_map.end(); it++)
// {
// 	delete(it->second);
// 	it->second = NULL;
// }
// _cgi_map.clear();
// std::cout << "Cleaning up CGI map..." << std::endl;
// for (std::map<int, t_CGI *>::iterator it = _cgi_map.begin(); it != _cgi_map.end(); it++)
// {
//     std::cout << "Deleting CGI at address: " << it->second << std::endl;
//     delete it->second;
//     it->second = NULL;
// }
// _cgi_map.clear();

//_fds.reserve(15100); //test

/////////////////////////////////////////////////////////////////////////////////////////////Testing only
void Core::acceptMockConnections(t_location& locate, t_request& request, int& clientCount)
{
    // Only create new clients if we are under our test limit
    if (clientCount < 10) 
    {
        int dummyFd = clientCount + 200;
        Client* client = new Client();
        
        // Setup CGI (simulation of a CGI request)
        client->setCgiExec(new CgiExecute(client, request, locate));
        
        // clientRegister should add the FD to your vector 
        // and set _fds[i].events = POLLIN
        clientRegister(dummyFd, client); 
        
        clientCount++;
    }
}
// if (clientCount < 10) //lower than 10
// {
// 	int dummyFd = clientCount + 200;
// 	// CgiRequest	requestor(request, locate);
// 	Client*		client = new Client();
// 	client->setCgiExec(new CgiExecute(client, request, locate)); //will be after i detect there is CGI
// 	clientRegister(dummyFd, client); //introduce to fdpoll struct
// 	clientCount++;
// }

//Testing
void Core::forceMockEvents()
{
    for (size_t i = 0; i < _fds.size(); i++)
    {
        // We look up the client associated with this specific FD
        Client* c = _clients[_fds[i].fd];

        // If the client exists and is waiting to be read, we 'fake' 
        // the activity that the OS would normally detect.
        if (c && c->state == READ_REQUEST) 
        {
            _fds[i].revents |= POLLIN; 
        }
    }
}
// for (size_t i = 0; i < _fds.size(); i++) //test to act as acceptor
// {
	// Client* c = _clients[_fds[i].fd];
	//If this is a dummy FD we want to force into action:
	// if (c && c->state == READ_REQUEST) 
	    // _fds[i].revents = POLLIN; // You are doing the OS's job here!
// }
//////////////////////////////////////////////////////////////////////////////////////////////////////