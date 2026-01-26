/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/25 23:14:02 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.h"
#include "CGI_data.h"
#include "CgiExecute.h"
#include "Respond.h"
#include <ctime>
#include <sys/poll.h>
#include <unistd.h>
#include <poll.h>
#include <iostream>
#include <cstdio>

Client::Client(t_server server_config) :  _executor(NULL), _socket(0), _hasCgi(false), _lastActivity(time(NULL)),
	 _connStatus(CLOSE), revived(false) //FromMuzz
{
	state = READ_REQUEST;
	_responder = new Respond();
	_responder->setClient(this);
	_serverConfig = server_config;
}

Client::~Client()
{
	// if (_requestor)
	// 	delete _requestor;
	if (_executor)
		delete _executor;
	if (_responder)
		delete _responder;
}

//i = index of Fds
//if POLLHUP for waitcgi means cgi finished and disconnected
//if POLLHUP for socket means socket disconnected
void	Client::procInput(int i, struct pollfd& pFd)
{
	(void)i;
	(void)pFd;
	// int pipeFromCgi = 0; // Initialize to avoid accessing NULL executor
	// if (_executor) {
	// 	pipeFromCgi = GetCgiExec()->getpipeFromCgi();
	// }
	std::cout << "trigger here" << std::endl; //debug
	std::cout << "state: " << state << std::endl; //debug
	if (state == HANDLE_REQUEST) //check
	{
		if (!_hasCgi)
		{
			// Set protocol from the parsed request - add safety check
			std::cout << "trigger here 1" << std::endl; //debug
			std::string protocol = request.http_version.empty() ? "HTTP/1.1" : request.http_version;
			getRespond().procNormalOutput(protocol);
			getRespond().buildResponse();
			state = SEND_RESPONSE;
		}
		else
		{
			// For CGI requests, we would need to create and configure the CGI executor
			// For now, just serve as normal file
			std::string protocol = request.http_version.empty() ? "HTTP/1.1" : request.http_version;
			std::cout << "content length: "  << request.content_length << std::endl; //debug
			setCgiExec(new CgiExecute(this, protocol));
			GetCgiExec()->preExecute();
			GetCgiExec()->execute();
			state = EXECUTE_CGI;
		}
	// // int pipeFromCgi = GetCgiExec()->getpipeFromCgi(); //my old commit
	//
	// if (state == READ_REQUEST)
	// {
	// 	//check client disconnect using received
	// 	//READ REQUEST fromMuzz
	// 	// if (/*read request finished*/)
	// 	// if (state == HANDLE_REQUEST)
	// 	// {
	// 		if (!isCGI(request, locate))
	// 		{
	// 			getRespond().procNormalOutput(request, locate);
	// 			getRespond().buildResponse();
	// 			state = SEND_RESPONSE;
	// 			// respondRegister(client);//
	// 			// state = WAIT_RESPONSE;
	// 		}
	// 		else
	// 		{
	// 			setCgiExec(new CgiExecute(this, request, locate));
	// 			GetCgiExec()->preExecute();
	// 			GetCgiExec()->execute();
	// 			state = EXECUTE_CGI;
	// 			// cgiRegister(client);//
	// 			// state = WAIT_CGI;
	// 		}
	// 	// }
	}
	else if (state == WAIT_CGI && pFd.fd == GetCgiExec()->getpipeFromCgi())
	{
		std::cout << "trigger here 13" << std::endl; //debug			
		GetCgiExec()->readExec();
		GetCgiExec()->cgiState();
		if (GetCgiExec()->isReadDone())
		{
			// fdPreCleanup(pipeFromCgi, i);//
			std::cout << "trigger here 14" << std::endl; //debug			
			std::cout << "cgi output: " << GetCgiExec()->getOutput() << std::endl;//debug
			fdPreCleanup(pFd);
			if (GetCgiExec()->isDone())
			{
				getRespond().setProtocol(request.http_version);
				getRespond().procCgiOutput(GetCgiExec()->getOutput());
				getRespond().buildResponse();
				state = SEND_RESPONSE;
			}
		}
	}
}

void	Client::procOutput(int i, struct pollfd& pFd)
{
	(void)i;
	(void)pFd;
	
	if (state == WAIT_CGI && _executor)
	{
		std::cout << "trigger here 11" << std::endl; //debug
		std::cout << "method: " << request.method << std::endl; //debug
		int pipeToCgi = GetCgiExec()->getpipeToCgi();
		if (pFd.fd == pipeToCgi) {
			GetCgiExec()->writeExec();
			GetCgiExec()->cgiState();
			if (GetCgiExec()->isWriteDone())
			{
				std::cout << "trigger here 12" << std::endl; //debug
				fdPreCleanup(pFd);
				if (GetCgiExec()->isDone())
				{
					getRespond().setProtocol(request.http_version);
					getRespond().procCgiOutput(GetCgiExec()->getOutput());
					getRespond().buildResponse();
					state = SEND_RESPONSE;
				}
			}
		}
	}
	if (state == WAIT_RESPONSE /*&& *fd == client->getSocket()*/)
	{
		int status = getRespond().sendResponse();
		if (status == -1)
		{
			state = DISCONNECTED;
			//client disconnect since send return -1 or 0
		}
		else if (status)
		{
			//getRespond().printResponse(); //important debug
			state = FINISHED;
		}
	}
}

//check 2 things
//1. cgi enabled? is the location for script to be run have permission for script
//2. executable name/suffix is correct? and can be execute?
bool	Client::isCGI(const t_request& request, const t_location& locate) const
{
	return (locate.cgi_enabled && isCGIextOK(request, locate));
}

//should at least support one. which we focus on .py
bool	Client::isCGIextOK(const t_request& request, const t_location& locate) const
{
	const std::string	path		= request.path;
	const std::string	interp	= locate.interp;
	
	//manually without taking from config cgi_ext
	//use rfind to detect the last dot
	size_t lastDot = path.rfind(".");
	if (lastDot == std::string::npos) //how to check npos
		return false; 
	//use path.substr and check
	std::string ext = path.substr(lastDot);
	if (ext != ".cgi" && ext != ".php" && ext != ".py"
		&& ext != ".sh" && ext != ".pl")
		return false;
	//1 case may need to handle but idk necessary or not.
	//./../../../etc/passwd.
	//but still considering to do this right after location matching or now.
	//bcus if now it might be redundant since static also may need it.
	//this will causes the user to go outside of root & get private info
	//if want to handle, use list and every node is separated by /. 
	//lets say meet .. pop back the last node.
	//then we create a string and compare with "var/www/html"
	return true;
}

void	Client::resetClient()
{
	//reset request struct/class FromMuzz
	if (_executor)
	{
		delete _executor;
		_executor = NULL;
	}
	_responder->resetResponder();
	_hasCgi = false;
	_lastActivity = time(NULL);
	_connStatus = KEEP_ALIVE;
	state = READ_REQUEST;
	revived = true; //testing
}

void	Client::fdPreCleanup(struct pollfd& pFd)
{
	close(pFd.fd);
	pFd.fd = -1;
}


int		Client::getSocket()
{
	return _socket;
}

void	Client::setSocket(int socket)
{
	_socket = socket;
}

void	Client::setCgiExec(CgiExecute* executor)
{
	_executor = executor;
}

// CgiRequest*		Client::GetCgiReq()
// {
// 	return _requestor;
// }	

CgiExecute*		Client::GetCgiExec()
{
	return _executor;
}

Respond&	Client::getRespond()
{
	return* _responder;
}

bool	Client::hasCgi()
{
	return _hasCgi;
}

bool	Client::isCgiOn()  //mcm x perlu
{
	return _executor->getpid();
}

void	Client::setHasCgi(bool status)
{
	_hasCgi = status;
}

bool	Client::isIdle(time_t now)
{
	if ((now - _lastActivity) > CLIENT_TIMEOUT)
		return true;
	return false;
}

bool	Client::isKeepAlive()
{
	return _connStatus;
}

void Client::setConnStatus(bool status)
{
    _connStatus = status;
}

std::string Client::readRawRequest()
{
	char buffer[10000];
	ssize_t bytes_read = read(_socket, buffer, sizeof(buffer) - 1);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';
		_lastActivity = time(NULL);
		std::cout << std::endl << "📥 Received " << bytes_read << " bytes from FD " << _socket << ":" << std::endl;
        std::cout << "\"" << std::string(buffer) << "\"" << std::endl;
        std::cout << "--- End Request ---" << std::endl;
		return (std::string(buffer));
	}
	else if (bytes_read == 0) {
		std::cout << "Client closed connection (FD: " << _socket << ")" << std::endl;
		return ("");
	}
	else {
		perror("Read error");
		return ("");
	}
}

s_HttpRequest& Client::getRequest() {
	return (request);
}

std::string Client::getRoot() {
	return (_serverConfig.root);
}

t_server	Client::getServerConfig()
{
	return (_serverConfig);
}

t_location&	Client::getCgiLocation()
{
	size_t i = 0;
	for (; i < _serverConfig.locations.size(); i++)
	{
		if (_serverConfig.locations[i].path == "/cgi-bin")
			return _serverConfig.locations[i];
	}
	return _serverConfig.locations[i]; //suppose not to trigger
}
