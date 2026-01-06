/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/05 22:32:22 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.h"
#include "CgiExecute.h"
#include <unistd.h>
#include <poll.h>
#include <iostream>

Client::Client() : state(READ_REQUEST),  _executor(NULL), _socket(0), _hasCgi(0)
{}

Client::~Client()
{
	// if (_requestor)
	// 	delete _requestor;
	if (_executor)
		delete _executor;
}

//i = index of Fds
void	Client::procInput(int i, struct pollfd& pFd)
{
	int pipeFromCgi = GetCgiExec()->getpipeFromCgi();

	if (state == READ_REQUEST)
	{
		//parser/config part.used to read http
		// if (/*read request finished*/)
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
		if (status)
		{
			getRespond().printResponse();
			state = FINISHED;
		}
	}
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

bool	Client::isCgiOn()
{
	return _hasCgi;
}

void	Client::setHasCgi(bool status)
{
	_hasCgi = status;
}
