/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/08 17:28:25 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Core.h"
#include "CGI_data.h"
#include <csignal>
#include <poll.h>
#include "CgiExecute.h"
#include "Client.h"
#include <iostream>
#include <unistd.h>
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

void	Core::run( t_location& locate, t_request& request)
{
	int			clientCount = 0;
	std::vector<Client*> activeClients;
	
	//listening socket into _fds FromMuzz
	int loopCount = 0;
	while (1)
	{
		acceptMockConnections(locate, request, clientCount);
		int result = poll(&_fds[0], _fds.size(), 4000);
		forceMockEvents();
		handleTimeout(); //better put before poll so poll dont run timeout client
		//					ACTUAL LOOP
		for (int i = 0; i < _fds.size(); i++)
		{
			int	oriFd = _fds[i].fd;
			if (oriFd == -1)
				continue ;
			int	revents = _fds[i].revents;
			Client* client = _clients[oriFd];
			// if (!revents)
			// 	continue;
			try
			{
				// if ((revents & (POLLERR | POLLNVAL)) && oriFd == client->getSocket()) // cause problem need to test with true socketfd
				// {
				// 	std::cout << "oriFd: " << oriFd << "socketFd: " << client->getSocket() << std::endl; //debug
				// 	std::cout << "revents: " << revents << "state client" << client->state << " d" << std::endl; //debug
				// 	deleteClient(client); //handle disconnect
				// 	continue ;
				// }
				if ((revents & POLLHUP) && oriFd == client->getSocket())
				{
					deleteClient(client); //handle disconnect;
					continue ;
				}
				if (revents & POLLIN || revents & POLLHUP)
				{
					//acceptor FromMuzz
					//create new client & struct FromMuzz
					client->procInput(i, _fds[i], request, locate));
				}
				if (_fds[i].revents & POLLOUT || revents & POLLHUP)
					client->procOutput(i, _fds[i]);
			}
			catch (int statusCode)
			{
				handleClientError(client, statusCode);
			}
			if (_fds[i].fd == -1)
			{
				_clients.erase(oriFd);
				_needCleanup = true;
			}
			//clean from maps/fd registration queue
			handleTransition(client);
		}
		//delete all in the delete list
		fdCleanup();
		addStagedFds(); //handle all sementara
		if(clientCount >= 10 && _clients.empty())
			break ;
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
		respondRegister(client);
		client->state = WAIT_RESPONSE;
	}
	else if (client->state == WAIT_RESPONSE /*&& *fd == client->getSocket()*/) //only for testing when no acceptor implemented
	{
		int status = client->getRespond().sendResponse();
		if (status)
		{
			client->getRespond().printResponse();
			client->state = FINISHED;
		}
	}
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
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); )
	{
		Client* client = it->second;
		
		std::map<int, Client*>::iterator current = it++;
		if (client->isIdle(now))
		{
			if (client->state == READ_REQUEST || client->state == FINISHED /*bufferRead == 0*/)
				deleteClient(client);
			else
				handleClientError(client, client->GetCgiExec()->getpid() ? 504 : 408);
		}
	}
}

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

	for (int i = 0; i < _fds.size(); i++)
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
// Testing only
void Core::acceptMockConnections(t_location& locate, t_request& request, int& clientCount)
{
    // Only create new clients if we are under our test limit
    if (clientCount < 10) 
    {
        int dummyFd = clientCount + 200;
        Client* client = new Client();
        
        // Setup CGI (simulation of a CGI request)
        // client->setCgiExec(new CgiExecute(client, request, locate));
        
        // clientRegister should add the FD to your vector 
        // and set _fds[i].events = POLLIN
        clientRegister(dummyFd, client); 
        
        clientCount++;
    }
}

//Testing
void Core::forceMockEvents()
{
    for (size_t i = 0; i < _fds.size(); i++)
    {
        // We look up the client associated with this specific FD
        Client* c = _clients[_fds[i].fd];

        // If the client exists and is waiting to be read, we 'fake' 
        // the activity that the OS would normally detect.
        if (c && c->state == READ_REQUEST && !c->revived) 
        {
            _fds[i].revents = POLLIN; 
        }
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