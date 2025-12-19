/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_execute.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 14:44:53 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/19 09:44:31 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI_data.h"
#include "CGI_request.h"
#include "CGI_execute.h"
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <cstring> //strdup
#include <sys/wait.h>
#include <iostream>
#include <poll.h>

CGI_execute::CGI_execute(const t_request& request, const t_location& locate)
	: _request(request), _locate(locate)
{}

CGI_execute::~CGI_execute()
{}

void	CGI_execute::execute()
{
	//pipe setup
	if (pipe(pipe_in) == -1)
		throw (std::runtime_error("CGI: pipe error"));
	if (pipe(pipe_out) == -1)
		throw (std::runtime_error("CGI: pipe error"));
	//fork
	pid = fork();
	if (pid == -1)
		throw (std::runtime_error("CGI: pipe error"));
	//child process
	else if (pid == 0)
	{
		//redirection
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		close(pipe_in[1]);
		close(pipe_out[0]);
		//execute CGI program
		char *args[] = {
    		const_cast<char*>(_locate.cgi_path.c_str()),  // argv[0]: The interpreter name
			const_cast<char*>(abs_path.c_str()),          // argv[1]: The script to run
    		NULL                         // Sentinel
			};
		//create envp. create a vector list. pushback everything needed from hardcoding
		std::vector<std::string> envp_vec;
		envp_vec.push_back("REQUEST_METHOD=" + _request.method); //REQUEST_METHOD
		envp_vec.push_back("QUERY_STRING=" + _request.query); //QUERY_STRING
		envp_vec.push_back("SCRIPT_NAME=" + _request.path); //SCRIPT_NAME
		envp_vec.push_back("SERVER_PROTOCOL=" + _request.version); //PROTOCOL VERSION
		//a logic for content depends on request headers
		std::map<std::string, std::string>::const_iterator it;
		for (it = _request.headers.begin(); it != _request.headers.end(); ++it)
		{
			std::string key = it->first;
			std::string value = it->second;
			std::string formattedkey = key;
			for (int i = 0; key[i]; i++)
			{
				formattedkey[i] = std::toupper(key[i]);
				if (formattedkey[i] == '-')
					formattedkey[i] = '_';
			}
			std::string name;
			if (formattedkey == "CONTENT_LENGTH" || formattedkey == "CONTENT_TYPE")
				name = formattedkey + "=" + value;
			else
				name = "HTTP_" + formattedkey + "=" + value;
			envp_vec.push_back(name);
		}
		//change vectors to array
		//then add HTTP_, upper every char and push every headers from struct to the envp list
		char** envp = new char*[envp_vec.size() + 1];
		for (size_t i = 0; i < envp_vec.size(); i++)
			envp[i] = strdup(envp_vec[i].c_str());
		envp[envp_vec.size()] = NULL;
		execve(const_cast<char*>(_locate.cgi_path.c_str()), args, envp);
		//add execve check
		std::cerr << "CGI error: execve failed" << std::endl;
		exit(1);
	}
	else
	{
		close(pipe_in[0]);
		close(pipe_out[1]);
		//Proceed with parent logic. write body to CGI
		t_CGI*  CGI = new t_CGI();
		CGI->pipeToCgi = pipe_in[1];
		CGI->clientSocket = 1; //hardcoded for now
		CGI->pid = pid;
		//read from CGI
		CGI->pipeFromCgi = pipe_out[0];
		//register in global map/struct
		data.map = 
		//add the pollfd vector

		//
	}
}

// char		read_buf[8192];
// ssize_t		read_len = 0;
// while((read_len = read(pipe_out[0], read_buf, sizeof(read_buf))) > 0)
// 	_output.append(read_buf, read_len);
// if (read_len == -1)
// 	throw (std::runtime_error("CGI: cannot read. pipe error")); // pipe error since nothing to read
// 	//wait for child
// 	int status;
// 	//pid_t res = waitpid(pid, &status, WNOHANG);
// 	// while (res == 0)
// 	// {
// 	// 	sleep(5);
// 	// }
// 	pid_t res = waitpid(pid, &status, 0);
// 	//add waitpid handling
// 	if (WIFEXITED(status))
// 	{
// 		int code = WEXITSTATUS(status);
// 		if (code !=  0)
// 		{
// 			std::cerr << "CGI error: exited with code" << code << std::endl;
// 			throw(std::runtime_error("CGI error: child process"));
// 		}
// 	}
// 	else if (WIFSIGNALED(status))
// 	{
// 		std::cerr << "CGI error: crash with code" << WTERMSIG(status) << std::endl;
// 		throw (std::runtime_error("CGI error: child process"));
// 	}
// }

//calculate abspath
//prepare envp, args, 
void	CGI_execute::preExecute()
{
	std::string script_name = _request.path.substr(_locate.path.size()); 
	std::string	root = _locate.root; //may not need
	std::string	cgi_path	= _locate.cgi_path;
	//script_name should be /test.py
	//_locate.root should be ./www
	//ensure locate.root doesnt end with /
	if (!_locate.root.empty() && _locate.root[_locate.root.size() - 1] == '/')
		root.erase(root.size() - 1,  1);
	//ensure script_name start with /
	if (script_name[0] != '/')
		script_name = "/" + script_name;
	abs_path = root + script_name;
	//check if file exist
	if (access(abs_path.c_str(), F_OK) == -1)
		throw(std::runtime_error("CGI error: no file permission"));
	//check if executable. do exist and execute check instead of exec check alone to send correct error message
	if (access(abs_path.c_str(), X_OK) == -1)
		throw(std::runtime_error("CGI error: no file permission"));
	if (access(cgi_path.c_str(), X_OK) == -1)
		throw(std::runtime_error("CGI error: no file permission"));
}

const std::string&	CGI_execute::getOutput() const
{
	return (_output);
}
