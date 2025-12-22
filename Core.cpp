/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/22 15:16:36 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Core.h"
#include "CGI_data.h"
#include <poll.h>
#include <stdexcept>
#include "CgiExecute.h"
#include "CgiRequest.h"
#include "Client.h"
#include <iostream>

Core::Core() {}

Core::~Core() 
{
	for (std::map<int, t_CGI *>::iterator it = _cgi_map.begin(); 
		it != _cgi_map.end(); it++)
	{
		delete(it->second);
	}
	_cgi_map.clear();
}

void	Core::run( t_location& locate, t_request& request)
{
	Client*		c = new Client();
	CgiRequest	requestor(request, locate);
	CgiExecute	executor(&client, request, locate);

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

void	Core::launchCgi(CgiExecute& executor, t_location& locate, t_request& request)
{
	executor.preExecute();
	executor.execute();
	//the part im trying to implement
	cgiRegister(executor.getCgiStruct()); //in core because it change the struct that hold all the list
	
}

void	Core::cgiWait(CgiExecute& executor)
{
	for(int i = 0; i < _fds.size(); i++)
	{
		if (_fds[i].revents == POLLIN)
		{
			executor.readExec();
			t_CGI* current = _cgi_map[_fds[i].fd];
		}
		if (_fds[i].revents == POLLOUT)
		{
			cgi
		}
		if (_fds[i].revents == POLLHUP)
		{
			
		}
		executor.cgiState(); //in cgi class because it change the state of the individual cgi
	}
}

void	Core::cgiRegister(t_CGI* cgiStruct)
{
	_cgi_map[cgiStruct->pipeToCgi] = cgiStruct;
	_cgi_map[cgiStruct->pipeFromCgi] = cgiStruct;
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
