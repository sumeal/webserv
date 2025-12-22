/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiExecute.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/22 00:37:30 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/22 14:41:50 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTE_H
#define CGIEXECUTE_H

#include "CGI_data.h"
#include <sched.h>

class Client;

class CgiExecute {
private:
	const t_request&	_request;
	const t_location&	_locate;
	t_CGI*		_cgi;
	Client*		_client;
	int		pipe_in[2];
	int		pipe_out[2];
	pid_t	pid;
	//for execve. 
    std::string abs_path;    // /var/www/html/cgi-bin/test.py
	//execve.CGI script(eg. py)
    std::string remote_addr; // 192.168.1.5
    int         server_port; // 8080
	std::string	_output;
public:
	CgiExecute(Client* client, const t_request& request, const t_location& locate);
	~CgiExecute();

	void	preExecute();
	void	execute();
	bool	isFinished() const;
	void	cleanup();
	void	readExec();
	void	cgiState();

	const std::string&	getOutput() const;
	t_CGI*	getCgiStruct() const;
};

#endif