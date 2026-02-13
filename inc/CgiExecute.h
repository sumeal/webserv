/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiExecute.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/22 00:37:30 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/13 15:23:53 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTE_H
#define CGIEXECUTE_H

#include "CGI_data.h"
#include <cstddef>
#include <sched.h>

class Client;

class CgiExecute {
private:
	s_HttpRequest _request;
	t_location _locate;
	Client*		_client;
	
	//CGI pre
	pid_t	_pid;
	int		_pipeIn[2];
	int		_pipeOut[2];
	int		_pipeToCgi;
	int		_pipeFromCgi;
    std::string _scriptPath; // /var/www/html/cgi-bin/test.py
	std::string	_protocol;
	
	//CGI post
	std::string	_output;
	size_t		_bodySizeSent;
	bool		_writeEnded;
	bool		_readEnded;
	int			_exitStatus;
public:
	CgiExecute(Client* client, std::string protocol);
	~CgiExecute();

	void	preExecute();
	void	execute();
	void	execChild();
	char**	createEnvp();
	void	readExec();
	void	writeExec();
	void	cgiState();
	void	clearCgi();

	bool	isCGI() const;
	bool	isCGIextOK() const;
	bool	isDone() const;
	bool	isReadDone() const;
	bool	isWriteDone() const;

	const std::string&	getOutput() const;
	int		getpipeToCgi();
	int		getpipeFromCgi();
	int		getpid() const;
	Client*	getClient();
};

#endif
