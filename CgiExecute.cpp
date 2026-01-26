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

// #include "CgiRequest.h"
#include "CGI_data.h"
#include "CgiExecute.h"
#include <cerrno>
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

// CgiExecute::CgiExecute(Client* client, const t_location& locate, const t_request& request)
// 	: _request(request), _locate(locate), _client(client), _pid(-1), _pipeToCgi(-1), 
// 	_pipeFromCgi(-1), _bodySizeSent(0), _writeEnded(false), _readEnded(false), 
// 	_exitStatus(0)
// {}

CgiExecute::CgiExecute(Client* client, std::string protocol)
	: _request(client->getRequest()), _locate(client->getCgiLocation()), _client(client), _pid(-1), _pipeToCgi(-1), 
	_pipeFromCgi(-1), _protocol(protocol), _bodySizeSent(0), _writeEnded(false), _readEnded(false), 
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
	_client->setHasCgi(true);
	close(_pipeIn[0]); close(_pipeOut[1]);
	_pipeToCgi = _pipeIn[1];
	_pipeFromCgi = _pipeOut[0];
	if (fcntl(_pipeToCgi, F_SETFL, O_NONBLOCK) == -1
		|| fcntl(_pipeFromCgi, F_SETFL, O_NONBLOCK) == -1)
		throw(500);
}

void	CgiExecute::execChild()
{
	dup2(_pipeIn[0], STDIN_FILENO);
	dup2(_pipeOut[1], STDOUT_FILENO);
	close(_pipeIn[1]); close(_pipeOut[0]);

	//				EXEC CGI
	// _locate.interp = "/usr/bin/python3"; //hardcode askMuzz
	char*	interpreter = const_cast<char*>(_locate.interp.c_str());
	std::cout << "locate interpreter path: " << _locate.interp  << std::endl; //debug
	char*	scriptPath = const_cast<char *>(_absPath.c_str());
	std::cout << "locate abs path: " << _absPath << std::endl; //debug
	char*	args[] = { interpreter, scriptPath,NULL};
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
		std::cout << "trigger here 4" << std::endl; //debug
		throw(502);
	}
}

void	CgiExecute::writeExec()
{
	std::cout << "trigger here 11a" << std::endl; //debug
	if (_request.body.empty() || _writeEnded)
	{
		std::cout << "trigger here 11aa" << std::endl; //debug
		if (_request.body.empty()) //debug
		{
			std::cout << "content_length: " << _request.content_length << std::endl; //debug
			std::cout << "_request body empty()" << std::endl; //debug
		}
		else
			std::cout << "write ended" << std::endl; //debug
		_writeEnded = true;
		return ;
	}
	std::cout << "trigger here 11b" << std::endl; //debug
	size_t	bytesLeft = _request.body.size() - _bodySizeSent;
	ssize_t	written = write(_pipeToCgi, _request.body.c_str() + _bodySizeSent, bytesLeft);
	if (written > 0)
	{
		std::string writtenstring = _request.body.substr(_bodySizeSent, written); //debug
		std::cout << "cgi written:  " <<  writtenstring << std::endl; //debug
		_bodySizeSent += written;
	}
	if (written == -1)
	{
		std::cout << "trigger here 11c" << std::endl; //debug
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
		throw(500);
	}
	if (_bodySizeSent == _request.body.size())
		_writeEnded = true;
	std::cout << "trigger here 11d" << std::endl; //debug
}

//calculate abspath
void	CgiExecute::preExecute()
{
	std::string script_name = _request.path.substr(_locate.path.size()); 
	std::string	root 		= _locate.root;
	size_t  dotPos = script_name.find_last_of(".");
	std::string ext = script_name.substr(dotPos);
	if (ext == ".py")
		_locate.interp = "/usr/bin/python3"; //hardcode askMuzz
	else if (ext == ".php")
		_locate.interp = "/usr/bin/php";
	std::string	interp	= _locate.interp;

	//script_name should be /test.py. locate.root should be ./www.
	//					SCRIPT START W /
	if (script_name[0] != '/')
		script_name = "/" + script_name;
	_absPath = root + _locate.path + script_name; 
	std::cout << "Abs Path: " << _absPath << std::endl; //debug
	std::cout << "root: " << root << std::endl; //debug
	std::cout << "interp: " << interp << std::endl; //debug
	std::cout << "cgi path: " << _locate.path << std::endl; //debug
	//					ROOT END NOT /
	if (!_locate.root.empty() && _locate.root[_locate.root.size() - 1] == '/')
		root.erase(root.size() - 1,  1);
	//					CHECK EXISTENCE
	if (access(_absPath.c_str(), F_OK) == -1)
		throw(404);
	//					CHECK EXISTENCE & EXEC
	if (access(_absPath.c_str(), X_OK) == -1)
	{
		std::cout << "trigger here 9" << std::endl; //debug
		throw(403);
	}
	if (access(interp.c_str(), X_OK) == -1)
	{
		std::cout << "interp check: " << std::endl; //debug
		std::cout << "trigger here 8" << std::endl; //debug
		throw(403);
	}
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
				if (WEXITSTATUS(status) == 0) //maybe can remove since default is 200
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
	if (/*_cgi->pid == -1 &&*/ _readEnded && _writeEnded)
		_client->state = SEND_RESPONSE;
}

//check 2 things
//1. cgi enabled? is the location for script to be run have permission for script
//2. executable name/suffix is correct? and can be execute?
bool	CgiExecute::isCGI() const
{
	return (_locate.cgi_enabled && isCGIextOK());
}

//should at least support one. which we focus on .py
bool	CgiExecute::isCGIextOK() const
{
	const std::string	path		= _request.path;
	const std::string	interp	= _locate.interp;
	
	//manually without taking from config cgi_ext
	//use rfind to detect the last dot
	size_t lastDot = path.rfind(".");
	if (lastDot == std::string::npos) //how to check npos
		return false; 
	//use path.substr and check
	std::string ext = path.substr(lastDot);
	if (ext != ".cgi" && ext != ".php" && ext != ".py"
		&& ext != ".sh" && ext != ".pl")
		return false;
	//1 case may need to handle but idk necessary or not.
	//./../../../etc/passwd.
	//but still considering to do this right after location matching or now.
	//bcus if now it might be redundant since static also may need it.
	//this will causes the user to go outside of root & get private info
	//if want to handle, use list and every node is separated by /. 
	//lets say meet .. pop back the last node.
	//then we create a string and compare with "var/www/html"
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
	// if (_pipeFromCgi != -1)
	// {
	// 	close(_pipeFromCgi);
	// 	_pipeFromCgi = -1;
	// }
	// if (_pipeToCgi != -1)
	// {
	// 	close(_pipeToCgi);
	// 	_pipeToCgi =  -1;
	// }
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