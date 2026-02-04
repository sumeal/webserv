/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abin-moh <abin-moh@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/04 14:23:28 by abin-moh         ###   ########.fr       */
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
#include <iostream>
#include <cstdio>
#include <cctype>

Client::Client(t_server& server_config, std::map<std::string, std::string>& cookies) : 
	_serverConfig(server_config), _executor(NULL), 
	_responder(_serverConfig, cookies), _socket(0), 
	_hasCgi(false), _lastActivity(time(NULL)), _connStatus(CLOSE), 
	_bestLocation(NULL), _headersParsed(false), _expectedBodyLength(0), 
	_currentBodyLength(0), _requestComplete(false), _disconnected(false), 
	_isChunked(false), _chunkedComplete(false), 
	_maxBodySize(server_config.client_max_body_size), revived(false)
{
	state = READ_REQUEST;
	_responder.setClient(this);
}

Client::~Client()
{
	if (_executor)
		delete _executor;
}

//i = index of Fds
//if POLLHUP for waitcgi means cgi finished and disconnected
//if POLLHUP for socket means socket disconnected
void	Client::procInput(int i, struct pollfd& pFd)
{
	(void)i;
	(void)pFd;
	if (state == HANDLE_REQUEST)
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
			setCgiExec(new CgiExecute(this, protocol));
			GetCgiExec()->preExecute();
			GetCgiExec()->execute();
			state = EXECUTE_CGI;
		}
	}
	else if (state == WAIT_CGI && pFd.fd == GetCgiExec()->getpipeFromCgi())
	{
		GetCgiExec()->readExec();
		GetCgiExec()->cgiState();
		if (GetCgiExec()->isReadDone())
		{
			// fdPreCleanup(pipeFromCgi, i);//
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
		int pipeToCgi = GetCgiExec()->getpipeToCgi();
		if (pFd.fd == pipeToCgi) {
			GetCgiExec()->writeExec();
			GetCgiExec()->cgiState();
			if (GetCgiExec()->isWriteDone())
			{
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
			getRespond().printResponse(); //importantdebug
			state = FINISHED;
		}
	}
}

//check 2 things
//1. cgi enabled? is the location for script to be run have permission for script
//2. executable name/suffix is correct? and can be execute?
bool	Client::isCGI(const s_HttpRequest& request, const t_location& locate) const
{
	return (locate.cgi_enabled && isCGIextOK(request, locate));
}

//should at least support one. which we focus on .py
bool	Client::isCGIextOK(const s_HttpRequest& request, const t_location& locate) const
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
	_responder.resetResponder();
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

CgiExecute*		Client::GetCgiExec()
{
	return _executor;
}

Respond&	Client::getRespond()
{
	return _responder;
}

bool	Client::getHasCgi()
{
	return _hasCgi;
}

bool	Client::isCgiExecuted()
{
	return _executor;
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

void Client::setMaxBodySize(size_t maxSize)
{
	_maxBodySize = maxSize;
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
		
		// std::cout << "📥 Received " << bytes_read << " bytes from FD " << _socket << std::endl;
		
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
				
				_isChunked = parseTransferEncoding(headers_only);
				
				if (_isChunked) {
					// std::cout << "📋 Headers parsed - CHUNKED transfer encoding detected" << std::endl;
					_expectedBodyLength = 0; // Unknown for chunked
				} else {
					_expectedBodyLength = parseContentLength(headers_only);
					if (_expectedBodyLength > _maxBodySize) {
						// std::cout << "Request body too large" <<std::endl;
						_requestComplete = false;
						_disconnected = true;
						throw(413);
					}
					// std::cout << "📋 Headers parsed - Expected body: " << _expectedBodyLength << " bytes";
				}
				
				_currentBodyLength = _rawBuffer.length() - header_end;
				// std::cout << ", Current: " << _currentBodyLength << " bytes" << std::endl;
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
		
		if (_headersParsed) {
			bool isComplete = false;
			
			if (_isChunked) {
				// Simple chunked completion check
				isComplete = isChunkedComplete();
			} else {
				// Normal Content-Length completion
				isComplete = (_currentBodyLength >= _expectedBodyLength);
			}
			
			if (isComplete) {
				_requestComplete = true;
				_currentRequest = _rawBuffer;
				
				if (_isChunked) {
					// std::cout << "✅ Complete chunked HTTP request received" << std::endl;
				} else {
					// std::cout << "✅ Complete HTTP request received (" << _rawBuffer.length() << " bytes)" << std::endl;
				}
				return true;
			}
		}
		
		return false;
	}
	else if (bytes_read == 0) {
		// std::cout << "📤 Client closed connection (FD: " << _socket << ")" << std::endl;
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
		_isChunked = false;
	_chunkedComplete = false;
	_chunkedBody.clear();
}

size_t Client::parseContentLength(const std::string& headers)
{
	size_t pos = 0;
	size_t line_start = 0;
	
	while (pos < headers.length()) {
		size_t line_end = headers.find('\n', pos);
		if (line_end == std::string::npos) {
			line_end = headers.length();
		}
		std::string line = headers.substr(line_start, line_end - line_start);
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
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
						// std::cout << "📏 Found Content-Length: " << value.substr(start) << std::endl;
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

bool Client::isChunkedComplete()
{
	// Simple chunked completion check - look for final chunk (0\r\n\r\n)
	if (!_isChunked) {
		return false;
	}
	
	// Find where body starts
	size_t body_start = _rawBuffer.find("\r\n\r\n");
	if (body_start == std::string::npos) {
		body_start = _rawBuffer.find("\n\n");
		if (body_start != std::string::npos) {
			body_start += 2;
		}
	} else {
		body_start += 4;
	}
	
	if (body_start == std::string::npos) {
		return false;
	}
	
	std::string body = _rawBuffer.substr(body_start);
	
	// Simple check for final chunk: look for "0\r\n\r\n" or "0\n\n"
	if (body.find("0\r\n\r\n") != std::string::npos || 
		body.find("0\n\n") != std::string::npos) {
		// std::cout << "🏁 Chunked request final chunk detected" << std::endl;
		return true;
	}
	
	return false;
}

bool Client::parseTransferEncoding(const std::string& headers)
{
	// std::cout << "🔍 Checking for Transfer-Encoding header..." << std::endl;
	// std::cout << "📋 Headers to check (" << headers.length() << " bytes):" << std::endl;
	// std::cout << "📄 Raw headers content:" << std::endl;
	// std::cout << "\"" << headers << "\"" << std::endl;
	// std::cout << "--- End Headers ---" << std::endl;
	
	// Simple chunked detection - minimal implementation
	size_t pos = 0;
	
	while (pos < headers.length()) {
		size_t line_end = headers.find('\n', pos);
		if (line_end == std::string::npos) {
			line_end = headers.length();
		}
		
		std::string line = headers.substr(pos, line_end - pos);
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
		
		// Print each header line for debugging
		// std::cout << "🔎 Header line: \"" << line << "\"" << std::endl;
		
		// Convert to lowercase for comparison
		std::string lower_line = line;
		for (size_t i = 0; i < lower_line.length(); i++) {
			lower_line[i] = std::tolower(lower_line[i]);
		}
		
		if (lower_line.find("transfer-encoding:") == 0) {
			// std::cout << "🎯 Found Transfer-Encoding header!" << std::endl;
			if (lower_line.find("chunked") != std::string::npos) {
				// std::cout << "✅ Chunked encoding detected!" << std::endl;
				return true;
			} else {
				// std::cout << "❌ Transfer-Encoding found but not chunked: \"" << line << "\"" << std::endl;
			}
			break;
		}
		
		pos = line_end + 1;
	}
	
	// std::cout << "❌ No Transfer-Encoding: chunked header found" << std::endl;
	return false;
}

std::string Client::readRawRequest()
{
	// Legacy method for backward compatibility
	char buffer[10000];
	ssize_t bytes_read = recv(_socket, buffer, sizeof(buffer) - 1, 0);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';
		_lastActivity = time(NULL);
		// std::cout << std::endl << "📥 Received " << bytes_read << " bytes from FD " << _socket << ":" << std::endl;
        // std::cout << "\"" << std::string(buffer) << "\"" << std::endl;
        // std::cout << "--- End Request ---" << std::endl;
		return (std::string(buffer));
	}
	else if (bytes_read == 0) {
		// std::cout << "Client closed connection (FD: " << _socket << ")" << std::endl;
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

t_server&	Client::getServerConfig()
{
	return (_serverConfig);
}

//what to compare to get the matching location. something from request?
void	Client::checkBestLocation()
{
	// std::cout << "check best location enter" << std::endl; //debug
	t_location*		bestLoc = NULL;
	size_t			bestLen = 0;
	s_HttpRequest&	request = getRequest();

	// std::cout << "Request Path: " << request.path << std::endl;//debug
	for (size_t i = 0; i < _serverConfig.locations.size();i++)
	{
		t_location& loc =  _serverConfig.locations[i];

		// std::cout << "server path: " << loc.path << std::endl; //debug
		// std::cout << "must same. location path: " << loc.path ; //debug
		// std::cout << "request path: " << request.path << std::endl; //debug
		if (request.path.compare(0, loc.path.size(), loc.path) == 0) // 1. img 2.img/
		{
			bool boundary = (request.path.size() == loc.path.size() //cases where match both. req: img and loc: img, /img and /img 
			|| (!loc.path.empty() && loc.path[loc.path.size() - 1] == '/') //cases where loc is img/ and req is only img/... .  so we dont care after loc img/.  why req is only img/? the compare filter it
			|| request.path[loc.path.size()] == '/'); //cases where loc: img (no /) will match w/ req: img/ or imga or img.... we only want to allow req: img/
			if (boundary && loc.path.length() > bestLen)
			{
				bestLen = loc.path.length();  
				bestLoc = &_serverConfig.locations[i];
				// std::cout << "best location path 1: " << bestLoc->path << std::endl;  //debug
				// std::cout << "inside if " << i << std::endl;//debug
			}
		}
	}
	if (!bestLoc)
		throw 404;
	if (getRequest().method != "GET" && getRequest().method != "POST" 
		&& getRequest().method != "DELETE")
		throw 501;
	// std::cout << "bestLoc: " << bestLoc->path << "get allowance: " << bestLoc->allow_get << std::endl;//debug
	if ((getRequest().method == "GET" && !bestLoc->allow_get) ||
		(getRequest().method == "POST" && !bestLoc->allow_post) || 
		(getRequest().method == "DELETE" && !bestLoc->allow_delete))
	{
		// std::cout << "405 throw trigger" << std::endl; //debug
		throw 405; //will throw crash server or handle that one client only
	}
 	_bestLocation = bestLoc;
	std::cout << "best location path: " << _bestLocation->path << std::endl; //debug
	std::cout << "root: " << _bestLocation->root << std::endl; //debug
	// std::cout << "check best location doesnt throw" << std::endl; //debug
}

t_location*		Client::getBestLocation()
{
	return _bestLocation;
}

// t_location&	Client::getCgiLocation()
// {
// 	size_t i = 0;
// 	for (; i < _serverConfig.locations.size(); i++)
// 	{
// 		if (_serverConfig.locations[i].path == "/cgi-bin")
// 			return _serverConfig.locations[i];
// 	}
// 	return _serverConfig.locations[i]; //suppose not to trigger
// }

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