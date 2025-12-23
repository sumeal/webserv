/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/23 15:50:34 by mbani-ya         ###   ########.fr       */
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
#include <set>

Core::Core() {}

Core::~Core() 
{
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
	std::set<t_CGI*> deletedPtrs;
	
	for (std::map<int, t_CGI*>::iterator it = _cgi_map.begin(); it != _cgi_map.end(); it++)
	{
		t_CGI* current = it->second;
		if (current != NULL && deletedPtrs.find(current) == deletedPtrs.end())
		{
			deletedPtrs.insert(current);
			delete(current);
		}
	}
	_cgi_map.clear();//redundant as this only being used when we clear vector but keep the heap object. now the heap object already gone no use
}

void	Core::run( t_location& locate, t_request& request)
{
	CgiRequest	requestor(request, locate);
	Client*		client = new Client(&requestor);
	CgiExecute	executor(client, request, locate);

	while (1)
	{
		std::cout << "looping" << std::endl;
		int result = poll(&_fds[0], _fds.size(), 5000);
		//if request state
		if(client->state == READ_REQUEST)
		{
			//parser/config part
			client->state = EXECUTE_CGI;
		}
		//if execute CGI state
		std::cout << client->state << std::endl;
		if (client->state == EXECUTE_CGI)
		{
			if (!requestor.isCGI())
				throw(std::runtime_error("CGI: request not CGI"));
			launchCgi(executor, locate, request);
			client->state = WAIT_CGI;
		}
		std::cout << client->state << std::endl;
		if (result > 0 && client->state == WAIT_CGI)
		{
			cgiWait(executor);
		}
		//if send output to client
		if (client->state == SEND_RESPONSE)
		{
			//for now
			std::cout << executor.getOutput() << std::endl;
			break; 
		}		
		if (client->state == FINISHED)
		{
			//clear/delete/break
			std::cout << executor.getOutput() << std::endl;
			delete(client);
			client = NULL;
			break; 
		}
		std::cout << "inside6" << std::endl;
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
		std::cout << "inside" << std::endl; //debug
		if (!_fds[i].revents)
			continue;
		std::cout << "inside2" << std::endl; //debug
		if (_fds[i].revents & (POLLIN | POLLHUP) && _fds[i].fd == executor.getCgiStruct()->pipeFromCgi)
		{
			executor.readExec();
			std::cout << "inside3" << std::endl; //debug
			executor.cgiState();
		}
		if (_fds[i].revents & POLLOUT && _fds[i].fd == executor.getCgiStruct()->pipeToCgi)
		{
			std::cout << "inside4" << std::endl; //debug
			executor.writeExec();
			executor.cgiState();
		}
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
