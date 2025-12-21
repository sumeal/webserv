/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgiExecute.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/22 00:37:30 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/22 00:45:34 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTE_H
#define CGIEXECUTE_H

#include "CGI_data.h"
#include <sched.h>

class cgiExecute {
private:
	const t_request&	_request;
	const t_location&	_locate;
	t_CGI*	_cgi;
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
	cgiExecute(const t_request& request, const t_location& locate);
	~cgiExecute();

	void	preExecute();
	void	execute();
	bool	isFinished() const;
	void	cleanup();
	void	readExec();

	const std::string&	getOutput() const;
	t_CGI*	getCgiStruct() const;
};

#endif