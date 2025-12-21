/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/22 01:02:03 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Core.hpp"
#include "CGI_data.h"
#include <poll.h>
#include <stdexcept>
#include "cgiExecute.h"
#include "CGI_request.h"
#include "Client.h"
#include <iostream>

Core::Core() {}

Core::~Core() 
{
	for (std::map<int, t_CGI *>::iterator it = cgi_map.begin(); 
		it != cgi_map.end(); it++)
	{
		delete(it->second);
	}
	cgi_map.clear();
}

void	Core::run( t_location& locate, t_request& request)
{
	CGI_request requestor(request, locate);
	cgiExecute	executor(request, locate);
	Client		client;

	while (1)
	{
		int result = poll(&_fds[0], _fds.size(), 5000);
		//if request state
		if(client.state == READ_REQUEST)
		{
			//parser/config part 
		}
		//if execute CGI state
		if (client.state == EXECUTE_CGI)
		{
			if (!requestor.isCGI())
				throw(std::runtime_error("CGI: request not CGI"));
			launchCgi(executor, locate, request);
			std::cout << executor.getOutput() << std::endl;
			client.state = WAIT_CGI;
		}
		if (result > 0 && client.state == WAIT_CGI)
		{
			cgiWait(executor);
		}
		//if send output to client
		if (client.state == FINISHED)
		{
			//clear/delete/break
		}
	}
}

void	Core::launchCgi(cgiExecute& executor, t_location& locate, t_request& request)
{
	executor.preExecute();
	executor.execute();
	//the part im trying to implement
	cgiRegister(executor.getCgiStruct());
	
}

void	Core::cgiRegister(t_CGI* cgiStruct)
{
	cgi_map[cgiStruct->pipeToCgi] = cgiStruct;
	cgi_map[cgiStruct->pipeFromCgi] = cgiStruct;
	struct pollfd pfdRead;
	pfdRead.fd = cgiStruct->pipeFromCgi;
	pfdRead.events = POLLIN;
	pfdRead.revents = 0;
	_fds.push_back(pfdRead);
	struct pollfd pfdWrite;
	pfdWrite.fd = cgiStruct->pipeToCgi;
	pfdWrite.events = POLLOUT;
	pfdWrite.revents = 0;
	_fds.push_back(pfdWrite);
}

void	Core::cgiWait(cgiExecute& executor)
{
	for(int i = 0; i < _fds.size(); i++)
	{
		if (_fds[i].revents == POLLIN)
		{
			executor.readExec();
			if (_cgi->readEnded && _cgi->writeEnded)
		}
		if (_fds[i].revents == POLLOUT)
		{
			cgi
		}
		if (_fds[i].revents == POLLHUP)
		{
			
		}
	}
}
