/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/30 15:10:38 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.h"
#include "CgiExecute.h"
#include <iostream>

Client::Client() : state(READ_REQUEST),  _executor(NULL)
{}

Client::~Client()
{
	// if (_requestor)
	// 	delete _requestor;
	if (_executor)
		delete _executor;
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
