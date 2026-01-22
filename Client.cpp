/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muzz <muzz@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/21 16:50:50 by muzz             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.h"
#include "CgiExecute.h"
#include "Respond.h"
#include <ctime>
#include <sys/poll.h>
#include <unistd.h>
#include <poll.h>
#include <iostream>
#include <cstdio>

Client::Client(t_server server_config) :  _executor(NULL), _socket(0), _hasCgi(false), _lastActivity(time(NULL)),
	 _connStatus(CLOSE) //FromMuzz
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
	int pipeFromCgi = 0; // Initialize to avoid accessing NULL executor
	if (_executor) {
		pipeFromCgi = GetCgiExec()->getpipeFromCgi();
	}

	if (state == HANDLE_REQUEST)
	{
		if (!_hasCgi)
		{
			// Set protocol from the parsed request - add safety check
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
			getRespond().procNormalOutput(protocol);
			getRespond().buildResponse();
			state = SEND_RESPONSE;
		}
	}
	else if (state == WAIT_CGI && pFd.fd == pipeFromCgi)
	{
		GetCgiExec()->readExec();
		GetCgiExec()->cgiState();
		if (GetCgiExec()->isReadDone())
		{
			// fdPreCleanup(pipeFromCgi, i);//
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
		int pipeToCgi = GetCgiExec()->getpipeToCgi();
		if (pFd.fd == pipeToCgi) {
			GetCgiExec()->writeExec();
			GetCgiExec()->cgiState();
			if (GetCgiExec()->isWriteDone())
			{
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

std::string Client::readRawRequest()
{
	char buffer[10000];
	ssize_t bytes_read = read(_socket, buffer, sizeof(buffer) - 1);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';
		_lastActivity = time(NULL);
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
