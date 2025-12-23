/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiExecute.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 14:44:53 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/22 00:37:50 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI_data.h"
#include "CgiRequest.h"
#include "CgiExecute.h"
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <unistd.h>
#include <cstring> //strdup
#include <sys/wait.h>
#include <iostream>
#include <poll.h>
#include <fcntl.h>
#include "Client.h"

CgiExecute::CgiExecute(Client* client, const t_request& request, const t_location& locate)
	: _client(client), _request(request), _locate(locate)
{}

CgiExecute::~CgiExecute()
{
	_cgi = NULL;
}

void	CgiExecute::execute()
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
		std::cerr << "CGI: execve error" << std::endl;
		for (int i = 0; i < envp_vec.size(); i++)
			free(envp[i]);
		delete[](envp);
		exit(1);
	}
	else
	{
		close(pipe_in[0]);
		close(pipe_out[1]);
		if (fcntl(pipe_in[1], F_SETFL, O_NONBLOCK) == -1)
			throw(std::runtime_error("CGI: pipe nonblock error"));
		if (fcntl(pipe_out[0], F_SETFL, O_NONBLOCK) == -1)
			throw(std::runtime_error("CGI: pipe nonblock error"));
		//Proceed with parent logic. write body to CGI
		_cgi = new t_CGI();
		_cgi->pipeToCgi = pipe_in[1];
		_cgi->clientSocket = 1; //hardcoded for now
		_cgi->pid = pid;
		//read from CGI
		_cgi->pipeFromCgi = pipe_out[0];
	}
}

void	CgiExecute::readExec()
{
	char	read_buf[8096];
	size_t	len  = 0;
	ssize_t	read_len;
	if ((read_len = read(pipe_out[0], read_buf, sizeof(read_buf))) > 0)
		_cgi->output.append(read_buf, read_len);
	if (read_len == 0)
	{
		_cgi->readEnded = 1;
		close(pipe_out[0]);
		_cgi->pipeFromCgi = -1; //best practice indicating closed
	}
	if (read_len == -1)
		throw(std::runtime_error("CGI: read error"));
}

void	CgiExecute::writeExec()
{
	if (_request.body.empty() || _cgi->writeEnded)
	{
		_cgi->writeEnded = true;
		return ;
	}
	size_t	bytesLeft = _request.body.size() - _cgi->bodySizeSent;
	ssize_t	written = write(pipe_in[1], _request.body.c_str() + _cgi->bodySizeSent, bytesLeft);
	if (written > 0)
		_cgi->bodySizeSent += written;
	if (written == -1)
		throw(std::runtime_error("CGI: write error"));
	if (_cgi->bodySizeSent == _request.body.size())
	{
		_cgi->writeEnded = true;
		close(_cgi->pipeToCgi);
		_cgi->pipeToCgi = -1; //best practice indicating closed
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
void	CgiExecute::preExecute()
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

void	CgiExecute::cgiState()
{
	if (_cgi->pid == -1)
		return ;
	int status;
	int res = waitpid(_cgi->pid, &status, WNOHANG);
	if (res == 0)
		return ;
	if (res > 0)
	{
		if (WIFEXITED(status))
		{
			if (WEXITSTATUS(status) == 0)
				_cgi->pid = -1;
			else if(WEXITSTATUS(status) != 0)
				throw (std::runtime_error("Waitpid WEXITSTATUS Error"));
		}
		else //WIFSIGNALED case
			throw (std::runtime_error("Waitpid WIFSIGNALED Error"));
	}
	if (res == -1)
		throw (std::runtime_error("Waitpid Error"));
	if (_cgi->readEnded && _cgi->writeEnded)
	{
		_client->state = SEND_RESPONSE;
	}
}

const std::string&	CgiExecute::getOutput() const
{
	return (_cgi->output);
}

t_CGI*	CgiExecute::getCgiStruct() const
{
	return (_cgi);
}