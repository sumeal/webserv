/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abin-moh <abin-moh@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/27 10:28:50 by abin-moh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.h"
#include "CGI_data.h"
#include "CgiExecute.h"
#include "Respond.h"
#include <ctime>
#include <sys/poll.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <errno.h>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cctype>

Client::Client(t_server server_config) :  _executor(NULL), _socket(0), _hasCgi(false), _lastActivity(time(NULL)),
	 _connStatus(CLOSE), _headersParsed(false), _expectedBodyLength(0), _currentBodyLength(0), 
	 _requestComplete(false), _disconnected(false), revived(false) //FromMuzz
{
	state = READ_REQUEST;
	_responder = new Respond();
	_responder->setClient(this);
	_serverConfig = server_config;
}

Client::~Client()
{
	// if (_requestor)
	// 	delete _requestor;
	if (_executor)
		delete _executor;
	if (_responder)
		delete _responder;
}

//i = index of Fds
//if POLLHUP for waitcgi means cgi finished and disconnected
//if POLLHUP for socket means socket disconnected
void	Client::procInput(int i, struct pollfd& pFd)
{
	(void)i;
	(void)pFd;
	// int pipeFromCgi = 0; // Initialize to avoid accessing NULL executor
	// if (_executor) {
	// 	pipeFromCgi = GetCgiExec()->getpipeFromCgi();
	// }
	if (state == HANDLE_REQUEST) //check
	{
		if (!_hasCgi)
		{
			// Set protocol from the parsed request - add safety check
			std::string protocol = request.http_version.empty() ? "HTTP/1.1" : request.http_version;
			getRespond().procNormalOutput(protocol);
			getRespond().buildResponse();
			state = SEND_RESPONSE;
		}
		else
		{
			// For CGI requests, we would need to create and configure the CGI executor
			// For now, just serve as normal file
			std::string protocol = request.http_version.empty() ? "HTTP/1.1" : request.http_version;
			std::cout << "content length: "  << request.content_length << std::endl; //debug
			setCgiExec(new CgiExecute(this, protocol));
			GetCgiExec()->preExecute();
			GetCgiExec()->execute();
			state = EXECUTE_CGI;
		}
	// // int pipeFromCgi = GetCgiExec()->getpipeFromCgi(); //my old commit
	//
	// if (state == READ_REQUEST)
	// {
	// 	//check client disconnect using received
	// 	//READ REQUEST fromMuzz
	// 	// if (/*read request finished*/)
	// 	// if (state == HANDLE_REQUEST)
	// 	// {
	// 		if (!isCGI(request, locate))
	// 		{
	// 			getRespond().procNormalOutput(request, locate);
	// 			getRespond().buildResponse();
	// 			state = SEND_RESPONSE;
	// 			// respondRegister(client);//
	// 			// state = WAIT_RESPONSE;
	// 		}
	// 		else
	// 		{
	// 			setCgiExec(new CgiExecute(this, request, locate));
	// 			GetCgiExec()->preExecute();
	// 			GetCgiExec()->execute();
	// 			state = EXECUTE_CGI;
	// 			// cgiRegister(client);//
	// 			// state = WAIT_CGI;
	// 		}
	// 	// }
	}
	else if (state == WAIT_CGI && pFd.fd == GetCgiExec()->getpipeFromCgi())
	{
		std::cout << "trigger here 13" << std::endl; //debug			
		GetCgiExec()->readExec();
		GetCgiExec()->cgiState();
		if (GetCgiExec()->isReadDone())
		{
			// fdPreCleanup(pipeFromCgi, i);//
			std::cout << "trigger here 14" << std::endl; //debug			
			std::cout << "cgi output: " << GetCgiExec()->getOutput() << std::endl;//debug
			fdPreCleanup(pFd);
			if (GetCgiExec()->isDone())
			{
				getRespond().setProtocol(request.http_version);
				getRespond().procCgiOutput(GetCgiExec()->getOutput());
				getRespond().buildResponse();
				state = SEND_RESPONSE;
			}
		}
	}
}

void	Client::procOutput(int i, struct pollfd& pFd)
{
	(void)i;
	(void)pFd;
	
	if (state == WAIT_CGI && _executor)
	{
		std::cout << "trigger here 11" << std::endl; //debug
		std::cout << "method: " << request.method << std::endl; //debug
		int pipeToCgi = GetCgiExec()->getpipeToCgi();
		if (pFd.fd == pipeToCgi) {
			GetCgiExec()->writeExec();
			GetCgiExec()->cgiState();
			if (GetCgiExec()->isWriteDone())
			{
				std::cout << "trigger here 12" << std::endl; //debug
				fdPreCleanup(pFd);
				if (GetCgiExec()->isDone())
				{
					getRespond().setProtocol(request.http_version);
					getRespond().procCgiOutput(GetCgiExec()->getOutput());
					getRespond().buildResponse();
					state = SEND_RESPONSE;
				}
			}
		}
	}
	if (state == WAIT_RESPONSE /*&& *fd == client->getSocket()*/)
	{
		int status = getRespond().sendResponse();
		if (status == -1)
		{
			state = DISCONNECTED;
			//client disconnect since send return -1 or 0
		}
		else if (status)
		{
			//getRespond().printResponse(); //important debug
			state = FINISHED;
		}
	}
}

//check 2 things
//1. cgi enabled? is the location for script to be run have permission for script
//2. executable name/suffix is correct? and can be execute?
bool	Client::isCGI(const t_request& request, const t_location& locate) const
{
	return (locate.cgi_enabled && isCGIextOK(request, locate));
}

//should at least support one. which we focus on .py
bool	Client::isCGIextOK(const t_request& request, const t_location& locate) const
{
	const std::string	path		= request.path;
	const std::string	interp	= locate.interp;
	
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

void	Client::resetClient()
{
	//reset request struct/class FromMuzz
	if (_executor)
	{
		delete _executor;
		_executor = NULL;
	}
	_responder->resetResponder();
	_hasCgi = false;
	_lastActivity = time(NULL);
	_connStatus = KEEP_ALIVE;
	state = READ_REQUEST;
	
	// Reset non-blocking request buffer
	resetRequestBuffer();
	
	revived = true; //testing
}

void	Client::fdPreCleanup(struct pollfd& pFd)
{
	close(pFd.fd);
	pFd.fd = -1;
}


int		Client::getSocket()
{
	return _socket;
}

void	Client::setSocket(int socket)
{
	_socket = socket;
}

void	Client::setCgiExec(CgiExecute* executor)
{
	_executor = executor;
}

// CgiRequest*		Client::GetCgiReq()
// {
// 	return _requestor;
// }	

CgiExecute*		Client::GetCgiExec()
{
	return _executor;
}

Respond&	Client::getRespond()
{
	return* _responder;
}

bool	Client::getHasCgi()
{
	return _hasCgi;
}

bool	Client::isCgiOn()  //mcm x perlu
{
	return _executor->getpid();
}

void	Client::setHasCgi(bool status)
{
	_hasCgi = status;
}

bool	Client::isIdle(time_t now)
{
	if ((now - _lastActivity) > CLIENT_TIMEOUT)
		return true;
	return false;
}

bool	Client::isKeepAlive()
{
	return _connStatus;
}

void Client::setConnStatus(bool status)
{
    _connStatus = status;
}

bool Client::readHttpRequest()
{
	char buffer[4096];
	ssize_t bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';
		_rawBuffer.append(buffer, bytes_read);
		_lastActivity = time(NULL);
		
		std::cout << "📥 Received " << bytes_read << " bytes from FD " << _socket << std::endl;
		
		if (!_headersParsed) {
			size_t header_end = _rawBuffer.find("\r\n\r\n");
			if (header_end == std::string::npos) {
				header_end = _rawBuffer.find("\n\n");
				if (header_end != std::string::npos) {
					header_end += 2;
				}
			} else {
				header_end += 4;
			}
			
			if (header_end != std::string::npos) {
				_headersParsed = true;
				
				std::string headers_only = _rawBuffer.substr(0, header_end);
				_expectedBodyLength = parseContentLength(headers_only);
				
				_currentBodyLength = _rawBuffer.length() - header_end;
				
				std::cout << "📋 Headers parsed - Expected body: " << _expectedBodyLength 
						  << " bytes, Current: " << _currentBodyLength << " bytes" << std::endl;
			}
		} else {
			_currentBodyLength = _rawBuffer.length() - (_rawBuffer.find("\r\n\r\n") + 4);
			if (_rawBuffer.find("\r\n\r\n") == std::string::npos) {
				size_t header_end = _rawBuffer.find("\n\n");
				if (header_end != std::string::npos) {
					_currentBodyLength = _rawBuffer.length() - (header_end + 2);
				}
			}
		}
		
		if (_headersParsed && _currentBodyLength >= _expectedBodyLength) {
			_requestComplete = true;
			_currentRequest = _rawBuffer;
			
			std::cout << "✅ Complete HTTP request received (" << _rawBuffer.length() << " bytes)" << std::endl;
			return true;
		}
		
		return false;
	}
	else if (bytes_read == 0) {
		std::cout << "📤 Client closed connection (FD: " << _socket << ")" << std::endl;
		_disconnected = true;
		_requestComplete = false;
		return false;
	}
	else {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return false;
		} else {
			perror("recv error");
			_requestComplete = false;
			return false;
		}
	}
}

bool Client::isRequestComplete()
{
	return _requestComplete;
}

bool Client::isDisconnected()
{
	return _disconnected;
}

std::string Client::getCompleteRequest()
{
	if (_requestComplete) {
		return _currentRequest;
	}
	return "";
}

void Client::resetRequestBuffer()
{
	_rawBuffer.clear();
	_currentRequest.clear();
	_headersParsed = false;
	_expectedBodyLength = 0;
	_currentBodyLength = 0;
	_requestComplete = false;
	_disconnected = false;
}

size_t Client::parseContentLength(const std::string& headers)
{
	// Manual parsing without getline() - fully non-blocking
	size_t pos = 0;
	size_t line_start = 0;
	
	while (pos < headers.length()) {
		// Find end of current line
		size_t line_end = headers.find('\n', pos);
		if (line_end == std::string::npos) {
			line_end = headers.length();
		}
		
		// Extract line (handle \r\n and \n)
		std::string line = headers.substr(line_start, line_end - line_start);
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
		
		// Check for Content-Length (case insensitive)
		if (line.length() > 14) { // "Content-Length" is 14 chars
			std::string lower_line = line;
			for (size_t i = 0; i < lower_line.length(); i++) {
				lower_line[i] = std::tolower(lower_line[i]);
			}
			
			if (lower_line.find("content-length:") == 0) {
				// Extract value after colon
				size_t colon_pos = line.find(':');
				if (colon_pos != std::string::npos) {
					std::string value = line.substr(colon_pos + 1);
					
					// Trim whitespace manually
					size_t start = 0;
					while (start < value.length() && 
						   (value[start] == ' ' || value[start] == '\t')) {
						start++;
					}
					
					if (start < value.length()) {
						std::cout << "📏 Found Content-Length: " << value.substr(start) << std::endl;
						return static_cast<size_t>(atoi(value.substr(start).c_str()));
					}
				}
				break;
			}
		}
		
		// Move to next line
		pos = line_end + 1;
		line_start = pos;
	}
	
	return 0; // No Content-Length found
}

std::string Client::readRawRequest()
{
	// Legacy method for backward compatibility
	char buffer[10000];
	ssize_t bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';
		_lastActivity = time(NULL);
		std::cout << std::endl << "📥 Received " << bytes_read << " bytes from FD " << _socket << ":" << std::endl;
        std::cout << "\"" << std::string(buffer) << "\"" << std::endl;
        std::cout << "--- End Request ---" << std::endl;
		return (std::string(buffer));
	}
	else if (bytes_read == 0) {
		std::cout << "Client closed connection (FD: " << _socket << ")" << std::endl;
		return ("");
	}
	else {
		perror("Read error");
		return ("");
	}
}

s_HttpRequest& Client::getRequest() {
	return (request);
}

std::string Client::getRoot() {
	return (_serverConfig.root);
}

t_server	Client::getServerConfig()
{
	return (_serverConfig);
}

t_location&	Client::getCgiLocation()
{
	size_t i = 0;
	for (; i < _serverConfig.locations.size(); i++)
	{
		if (_serverConfig.locations[i].path == "/cgi-bin")
			return _serverConfig.locations[i];
	}
	return _serverConfig.locations[i]; //suppose not to trigger
}
