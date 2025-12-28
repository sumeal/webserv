/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/26 12:51:26 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.h"
#include <iostream>

Client::Client(CgiRequest* requestor) : state(READ_REQUEST),  _requestor(requestor)
{}

Client::~Client()
{
	if (_requestor)
		delete _requestor;
	if (_executor)
		delete _executor;
	if (_cgi)
		delete _cgi;
	// std::cout << "deleted client" << std::endl;
}

t_CGI* Client::getCgi()
{
	return (_cgi);
}

int		Client::getSocket()
{
	return _socket;
}

void	Client::setSocket(int socket)
{
	_socket = socket;
}

void Client::setCgi(t_CGI* cgi)
{
	_cgi = cgi;
}

void	Client::setCgiExec(CgiExecute* executor)
{
	_executor = executor;
}

CgiRequest*		Client::GetCgiReq()
{
	return _requestor;
}	

CgiExecute*		Client::GetCgiExec()
{
	return _executor;
}
