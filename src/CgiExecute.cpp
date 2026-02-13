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

#include "./../inc/CGI_data.h"
#include "./../inc/CgiExecute.h"
#include "./../inc/Client.h"
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <sys/poll.h>
#include <unistd.h>
#include <cstring> //strdup
#include <sys/wait.h>
#include <iostream>
#include <poll.h>
#include <fcntl.h>
// #include <stdexcept>

CgiExecute::CgiExecute(Client* client, std::string protocol)
	: _request(client->getRequest()), _locate(*client->getBestLocation()), _client(client), _pid(-1), _pipeToCgi(-1), 
	_pipeFromCgi(-1), _protocol(protocol), _output(""), _bodySizeSent(0), _writeEnded(false), _readEnded(false), 
	_exitStatus(0)
{}

CgiExecute::~CgiExecute()
{}

void	CgiExecute::execute()
{
	//				PIPE & FORK
	if (pipe(_pipeIn) == -1)
		throw (500);
	if (pipe(_pipeOut) == -1)
	{
		close(_pipeIn[0]); close(_pipeIn[1]);
		throw (500);
	}
	_pid = fork();
	if (_pid == -1)
	{
		close(_pipeIn[0]); close(_pipeIn[1]);
		close(_pipeOut[0]); close(_pipeOut[1]);
		throw (500);
	}
	else if (_pid == 0)
		execChild();
	//			NON-BLOCKING PIPE AND STORE RESULT
	close(_pipeIn[0]); close(_pipeOut[1]);
	_pipeToCgi = _pipeIn[1];
	_pipeFromCgi = _pipeOut[0];
	if (fcntl(_pipeToCgi, F_SETFL, O_NONBLOCK) == -1
		|| fcntl(_pipeFromCgi, F_SETFL, O_NONBLOCK) == -1)
		{
			throw(500);
		}
}

void	CgiExecute::execChild()
{
	dup2(_pipeIn[0], STDIN_FILENO);
	dup2(_pipeOut[1], STDOUT_FILENO);
	close(_pipeIn[1]); close(_pipeOut[0]);

	//				MOVE TO CGI DIRECTORY //changed
	size_t lastSlash = _scriptPath.find_last_of("/");
	if (lastSlash != std::string::npos)
	{
		std::string cgiDir	= _scriptPath.substr(0, lastSlash);
		std::string file	= _scriptPath.substr(lastSlash + 1); 
		if (chdir(cgiDir.c_str()) == -1)
		{
			std::cerr << "CGI Error: Could not change directory to " << cgiDir << std::endl;
			exit (1);
		}
	}
	//				EXEC CGI
	char*	interpreter = const_cast<char*>(_locate.interp.c_str());
	std::string file	= _scriptPath.substr(lastSlash + 1);
	char*	scriptPath = const_cast<char *>(file.c_str());
	char*	args[] = {interpreter, scriptPath, NULL};
	char**	envp = createEnvp();
	execve(interpreter, args, envp);

	//				HANDLE ERROR     
	std::cerr << "CGI: execve error" << std::endl;
	for (int i = 0; envp[i]; i++)
		free(envp[i]);
	delete[](envp);
	exit(1);
}

//may need to implement PATH_INFO if evaluator say so
//eg."/cgi-bin/test.py/extra"
//extra is the PATH info
char**	CgiExecute::createEnvp()
{
	std::vector<std::string> envpVector;
	envpVector.push_back("REQUEST_METHOD=" + _request.method);
	envpVector.push_back("QUERY_STRING=" + _request.query);
	envpVector.push_back("SCRIPT_NAME=" + _request.path);
	envpVector.push_back("SERVER_PROTOCOL=" + _protocol); //PROTOCOL VERSION
	if (!_client->getRequest().cookie.empty())
		envpVector.push_back("HTTP_COOKIE=" + _client->getRequest().cookie);
	if (_locate.interp == "/usr/bin/php-cgi")
	{
		envpVector.push_back("REDIRECT_STATUS=200");
		envpVector.push_back("GATEWAY_INTERFACE=CGI/1.1");
		size_t lastSlash = _scriptPath.find_last_of("/");
		std::string file	= _scriptPath.substr(lastSlash + 1);
		std::string	scriptPath = file;
		envpVector.push_back("SCRIPT_FILENAME=" + scriptPath);
	}
	//				FORMAT HEADER
	//add HTTP_, upper char & push every headers from struct to the envp list
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
		envpVector.push_back(name);
	}

	//				VECTOR to ARRAY
	char** envp = new char*[envpVector.size() + 1];
	for (size_t i = 0; i < envpVector.size(); i++)
		envp[i] = strdup(envpVector[i].c_str());
	envp[envpVector.size()] = NULL;
	return envp;
}

void	CgiExecute::readExec()
{
	char	read_buf[8096];
	size_t	len  = 0;
	(void)len;
	ssize_t	readLen;
	if ((readLen = read(_pipeFromCgi, read_buf, sizeof(read_buf))) > 0)
		_output.append(read_buf, readLen);
	if (readLen == 0)
		_readEnded = 1;
	if (readLen == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;

		throw(502);
	}
}

void	CgiExecute::writeExec()
{
	if (_request.body.empty() || _writeEnded)
	{
		_writeEnded = true;
		return ;
	}
	size_t	bytesLeft = _request.body.size() - _bodySizeSent;
	ssize_t	written = write(_pipeToCgi, _request.body.c_str() + _bodySizeSent, bytesLeft);
	if (written > 0)
		_bodySizeSent += written;
	if (written == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
		throw(500);
	}
	if (_bodySizeSent == _request.body.size())
		_writeEnded = true;
}

//calculate abspath
void	CgiExecute::preExecute()
{
	std::string script_name = _request.path.substr(_locate.path.size());
	std::string	root 		= _locate.root;
	size_t  dotPos = script_name.find_last_of(".");
	if (dotPos == std::string::npos)
		throw(404);
	std::string ext = script_name.substr(dotPos);
	if (ext == ".py")
		_locate.interp = "/usr/bin/python3";
	else if (ext == ".php")
		_locate.interp = "/usr/bin/php-cgi";
	std::string	interp	= _locate.interp;

	//script_name should be /test.py. locate.root should be ./www.
	//					ROOT END NOT
	if (!_locate.root.empty() && _locate.root[_locate.root.size() - 1] == '/')
		root.erase(root.size() - 1,  1);
	//					SCRIPT START W /
	if (script_name[0] != '/')
		script_name = "/" + script_name;
	_scriptPath = root + _locate.path + script_name;
	//					CHECK EXISTENCE
	if (access(_scriptPath.c_str(), F_OK) == -1)
	{
		throw(404);
	}
	//					CHECK EXISTENCE & EXEC
	if (access(_scriptPath.c_str(), X_OK) == -1)
		throw(403);
	if (access(interp.c_str(), X_OK) == -1)
		throw(403);
}

void	CgiExecute::cgiState()
{
	if (_pid != -1)
	{
		int status;
		int res = waitpid(_pid, &status, WNOHANG);
		if (res == 0)
			return ;
		_pid = -1; //the point say it is finished successfully/wrongly
		if (res > 0)
		{
			if (WIFEXITED(status))
			{
				if (WEXITSTATUS(status) == 0) //can remove since default is 200
					_exitStatus = 200;
				if (WEXITSTATUS(status))
					_exitStatus = 500;
			}
			else //WIFSIGNALED case
				_exitStatus = 500;
		}
		else if (res == -1)
			_exitStatus = 500;
	}
	if (_readEnded && _writeEnded)
		_client->state = SEND_RESPONSE;
}

//check 2 things
//1. cgi enabled? is the location for script to be run have permission for script
//2. executable name/suffix is correct? and can be execute?
bool	CgiExecute::isCGI() const
{
	return (_locate.cgi_enabled && isCGIextOK());
}

bool	CgiExecute::isCGIextOK() const
{
	const std::string	path		= _request.path;
	const std::string	interp	= _locate.interp;
	
	size_t lastDot = path.rfind(".");
	if (lastDot == std::string::npos)
		return false; 
	std::string ext = path.substr(lastDot);
	if (ext != ".cgi" && ext != ".php" && ext != ".py"
		&& ext != ".sh" && ext != ".pl")
		return false;
	return true;
}

void	CgiExecute::clearCgi()
{
	if (_pid > 0)
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, WNOHANG);
		_pid = -1;
	}
}

const std::string&	CgiExecute::getOutput() const
{
	return (_output);
}

Client*	CgiExecute::getClient()
{
	return(_client);
}

int		CgiExecute::getpipeToCgi()
{
	return (_pipeToCgi);
}

int		CgiExecute::getpipeFromCgi()
{
	return (_pipeFromCgi);
}

bool	CgiExecute::isDone() const
{
	return (_readEnded && _writeEnded);
}

bool	CgiExecute::isReadDone() const
{
	return (_readEnded);
}

bool	CgiExecute::isWriteDone() const
{
	return (_writeEnded);
}

int	CgiExecute::getpid() const
{
	return _pid;
}