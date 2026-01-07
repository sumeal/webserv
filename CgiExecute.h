/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiExecute.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/22 00:37:30 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/06 21:59:20 by mbani-ya         ###   ########.fr       */
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
	//Client data
	const t_request&	_request;
	const t_location&	_locate;
	// t_CGI*		_cgi;
	Client*		_client;
	
	//Object data
	//CGI pre
	pid_t	_pid;
	int		_pipeIn[2];
	int		_pipeOut[2];
	int		_pipeToCgi;
	int		_pipeFromCgi;
    std::string _absPath;    // /var/www/html/cgi-bin/test.py
    std::string _remoteAddr; // 192.168.1.5.  what is this for??
    int         _serverPort; // 8080
	//CGI postresult
	std::string	_output;
	size_t		_bodySizeSent;
	bool		_writeEnded;
	bool		_readEnded;
	int			_exitStatus;
	//execve.CGI script(eg. py)
public:
	CgiExecute(Client* client, const t_request& request, const t_location& locate);
	~CgiExecute();

	void	preExecute();
	void	execute();
	void	execChild();
	char**	createEnvp();
	void	readExec();
	void	writeExec();
	void	cgiState();
	void	clearCgi();
	void	cleanup(); //check

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

// t_CGI*	getCgiStruct() const;