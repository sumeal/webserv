/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/07 23:40:58 by mbani-ya         ###   ########.fr       */
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

Client::Client() : state(READ_REQUEST),  _executor(NULL), _socket(0), 
	_hasCgi(false), _lastActivity(time(NULL)), _connStatus(KEEP_ALIVE) //FromMuzz
{}

Client::~Client()
{
	// if (_requestor)
	// 	delete _requestor;
	if (_executor)
		delete _executor;
}

//i = index of Fds
//if POLLHUP for waitcgi means cgi finished and disconnected
//if POLLHUP for socket means socket disconnected
void	Client::procInput(int i, struct pollfd& pFd)
{
	int pipeFromCgi = GetCgiExec()->getpipeFromCgi();

	if (state == READ_REQUEST)
	{
		//check client disconnect using received
		//READ REQUEST fromMuzz
		// if (/*read request finished*/)
		// if (state == HANDLE_REQUEST)
		// {
			if (!GetCgiExec()->isCGI())
			{
				getRespond().procNormalOutput();
				getRespond().buildResponse();
				state = SEND_RESPONSE;
				// respondRegister(client);//
				// state = WAIT_RESPONSE;
			}
			else
			{
				GetCgiExec()->preExecute();
				GetCgiExec()->execute();
				state = EXECUTE_CGI;
				// cgiRegister(client);//
				// state = WAIT_CGI;
			}
		// }
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
				getRespond().procCgiOutput(GetCgiExec()->getOutput());
				getRespond().buildResponse();
				state = SEND_RESPONSE;
			}
		}
	}
}

void	Client::procOutput(int i, struct pollfd& pFd)
{
	int pipeToCgi = GetCgiExec()->getpipeToCgi();
	
	if (state == WAIT_CGI && pFd.fd == pipeToCgi)
	{
		GetCgiExec()->writeExec();
		GetCgiExec()->cgiState();
		if (GetCgiExec()->isWriteDone())
		{
			fdPreCleanup(pFd);
			if (GetCgiExec()->isDone())
			{
				getRespond().procCgiOutput(GetCgiExec()->getOutput());
				getRespond().buildResponse();
				state = SEND_RESPONSE;
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
			getRespond().printResponse(); //important debug
			state = FINISHED;
		}
	}
}

void	Client::resetClient()
{
	//reset request struct/class FromMuzz
	if (_executor)
		delete _executor;
	_responder.resetResponder();
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

Respond&		Client::getRespond()
{
	return _responder;
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
