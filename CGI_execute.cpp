/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_execute.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 14:44:53 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/16 19:05:34 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI_request.h"
#include "CGI_execute.h"
#include <unistd.h>

bool	CGI_execute::execute()
{
	//pipe setup
	if (pipe(pipe_in) == -1)
		throw();
	if (pipe(pipe_out) == -1)
		throw();
	//fork
	pid = fork();
	if (pid == -1)
		throw();
	//child process
	else if (pid == 0)
	{
		//redirection
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		close(pipe_in[1]);
		close(pipe_out[0]);
		//execute CGI program
	}
	else
	{
		dup2(pipe_in[1], STDOUT_FILENO);
		dup2(pipe_out[0], STDIN_FILENO);
		close(pipe_in[0]);
		close(pipe_out[1]);
		//Proceed with parent logic
	}
}
