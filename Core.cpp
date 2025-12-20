/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:40:56 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/20 09:13:43 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Core.hpp"
#include "CGI_data.h"
#include <poll.h>

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

void	Core::launchCgi(CGI_execute& cgiExec, t_location& locate, t_request& request)
{
		cgiExec.preExecute();
		cgiExec.execute();
		//the part im trying to implement
		cgiRegister(cgiExec.getCgiStruct());

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