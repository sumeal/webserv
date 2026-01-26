/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 17:17:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/26 00:05:00 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Respond.h"
#include "Client.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <variant>
#include <sys/types.h> //ssize_t
#include <sys/socket.h> //send
#include <iostream>

//initialize the status code
Respond::Respond() : _client(NULL), _statusCode(0), _protocol("HTTP/1.1"), _contentLength(0), _serverName("localhost"), 
	_connStatus(KEEP_ALIVE), _socketFd(0), 
	_bytesSent(0)
{}

Respond::~Respond()
{}

//first line check?
void	Respond::procCgiOutput(std::string cgiOutput)
{
	if (cgiOutput.empty())
	{
		std::cout << "trigger here 5" << std::endl; //debug
		buildErrorResponse(502);
		return ;
	}
	//			FIND SEPARATOR(usually \r\n\r\n)
	size_t	separatorPos = cgiOutput.find("\r\n\r\n");
	size_t	offset = 4;
	if (separatorPos == std::string::npos)
	{
		separatorPos = cgiOutput.find("\n\n");
		offset = 2;
	}
	if (separatorPos == std::string::npos)
	{
		std::cout << "trigger here 6" << std::endl; //debug
		buildErrorResponse(502);
		return ;
	}
	//				EXTRACT HEADER		
	std::string	header = cgiOutput.substr(0, separatorPos);
	std::string headerLow = header;
	for (size_t i = 0; i < headerLow.length(); i++)
		headerLow[i] = std::tolower(headerLow[i]);
	//				FIND STATUS
	size_t	statusPos = headerLow.find("status");
	if (statusPos == std::string::npos)
		_statusCode = 200;
	else
	{
		// std::string statusStr = header.substr(statusPos + 8, 3);
		size_t	statusDigitPos = header.find_first_of("1234567890", statusPos);
		std::string statusStr = header.substr(statusDigitPos, 3);
		_statusCode = std::atoi(statusStr.c_str());
	}
	if (_statusCode < 100 || _statusCode > 599)
	{
		std::cout << "trigger here 7" << std::endl; //debug
		_statusCode = 502;
	}
	//				FIND CONTENT TYPE
	size_t contentTypePos = headerLow.find("content-type");
	if (contentTypePos != std::string::npos)
	{
		size_t	start = contentTypePos + 13;
		while (start < header.length() && (header[start] == ' ' || header[start] == '\t'))
			start++;
		size_t	end = header.find("\n", start); //by searching \n, can handle both
		if (end == std::string::npos)
			_contentType = header.substr(start);
		else
		{
			_contentType = header.substr(start, end - start);
			if (!_contentType.empty() && _contentType[_contentType.size() - 1] == '\r')
        		_contentType.erase(_contentType.size() - 1); //need to recheck
		}
	}
	else
		_contentType = "text/html";
	//				EXTRACT BODY	
	_body = cgiOutput.substr(separatorPos + offset);
	_contentLength = _body.length();
}
//chatgpt
void Respond::setSocketFd(int socketFd)
{
	_socketFd = socketFd;
}

// void	Respond::procNormalOutput(const t_request& request, const t_location& locate)
// {
// 	std::string script_name = request.path.substr(locate.path.size()); 
// 	std::string	root 		= locate.root;

// 	//script_name should be /test.py. locate.root should be ./www.
// 	//					SCRIPT START W /
// 	if (script_name[0] != '/')
// 		script_name = "/" + script_name;
// 	std::string absPath = root + script_name; 
// 	//					ROOT END NOT /
// 	if (!root.empty() && root[root.size() - 1] == '/')
// 		root.erase(root.size() - 1,  1);
	
// 	_filePath = absPath; //FromMuzz
// 	if (request.method == "POST")
// 	{
// 		std::ofstream outfile(_filePath.c_str(), std::ios::out | std::ios::trunc);
// 		if (!outfile.is_open())
// 			throw (500);
// 		outfile << request.body;
// 		outfile.close();
// 		_statusCode = 201;
// 		_body = "<html><head><title>201 Created</title></head><body>"
//         "<h1>File Created Successfully</h1><p>The resource has been uploaded.</p>"
//         "<hr><address>Webserv/1.0</address></body></html>";
// 		_contentLength = _body.size();
// 		_contentType = "text/html";
// 	}
// 	else if (request.method == "GET")
// 	{
// 		//read file and take path
// 		std::ifstream file(_filePath.c_str());
// 		if (!file.is_open())
// 			throw(404);
// 		std::stringstream ss;
// 		ss << file.rdbuf();
// 		//set status
// 		_statusCode = 200;
// 		//set body
// 		_body = ss.str();
// 		_contentLength = _body.size();
// 		//set content type based on the file extension
// 		setContentType();
// 	}
// }

void	Respond::setContentType()
{
	size_t pos = _filePath.rfind(".");
	if (pos != std::string::npos)
	{
		std::string ext = _filePath.substr(pos);
		if (ext == ".html")
			_contentType = "text/html";
		else if (ext == ".css")
			_contentType = "text/css";
		else if (ext == ".js")
			_contentType = "application/javascript";
		else if (ext == ".jpeg")
			_contentType = "image/jpeg";
		else if (ext == ".png")
			_contentType = "image/png";
		else
		 	_contentType = "text/plain"; //it wouldnt try to execute
	}
	else
	{
		//trigger to save/downlaod the file only
		_contentType = "application/octet-stream";
		//throw (404);
	}
}

void Respond::setClient(Client* client)
{
	_client = client;
}

std::string Respond::getRequestPath() {
	if (_client)
		return (_client->getRequest().path);
	return ("/");
}

void Respond::setContentType(const std::string& filePath)
{
    size_t pos = filePath.rfind(".");
    if (pos != std::string::npos) {
        std::string ext = filePath.substr(pos);
        
        if (ext == ".html" || ext == ".htm") {
            _contentType = "text/html";
        } else if (ext == ".css") {
            _contentType = "text/css";
        } else if (ext == ".js") {
            _contentType = "application/javascript";
        } else if (ext == ".json") {
            _contentType = "application/json";
        } else if (ext == ".png") {
            _contentType = "image/png";
        } else if (ext == ".jpg" || ext == ".jpeg") {
            _contentType = "image/jpeg";
        } else if (ext == ".gif") {
            _contentType = "image/gif";
        } else if (ext == ".txt") {
            _contentType = "text/plain";
        } else {
            _contentType = "application/octet-stream"; // Default binary
        }
    } else {
        _contentType = "text/plain"; // No extension
    }
}

//muzz why change
void	Respond::procNormalOutput(std::string protocol)
{
	std::string requestPath = getRequestPath();
	std::string documentRoot = getServerRoot();
	std::string filePath;

	if (requestPath == "/" || requestPath.empty())
		filePath = documentRoot + "/index.html";
	else
		filePath = documentRoot + requestPath;
	std::cout << _client->getRequest().method << std::endl; //debug
	if (_client->getRequest().method == "GET")
	{
		std::ifstream file(filePath.c_str());
		if (!file.is_open())
		{
			handleError(404);
			return;
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		file.close();
		
		_body = buffer.str();
		_statusCode = 200;
		_protocol = protocol;
		_contentLength = _body.size();

		setContentType(filePath);
		
		std::cout << "✅ " <<  filePath << " served"  << std::endl;
	} 
	else if (_client->getRequest().method == "POST")
	{
		std::ofstream outfile(_filePath.c_str(), std::ios::out | std::ios::trunc);
		if (!outfile.is_open())
			throw (500);
		outfile << _client->getRequest().body;
		outfile.close();
		_statusCode = 201;
		_body = "<html><head><title>201 Created</title></head><body>"
		"<h1>File Created Successfully</h1><p>The resource has been uploaded.</p>"
		"<hr><address>Webserv/1.0</address></body></html>";
		_contentLength = _body.size();
		_contentType = "text/html";		
	}
	else if (_client->getRequest().method == "DELETE")
	{
		if (std::remove(filePath.c_str()) == 0)
		{
			_statusCode = 204;
			_body.clear();
			_contentLength = 0;
			_contentType.clear();
		}
		else 
		{
			if (errno == ENOENT)
				throw (404);
			else if (errno == EACCES)
				throw (403);
			else
				throw (500);
		}
	}
}
	
	// std::string filePath = "FromMuzz";
	// //read file and take path
	// std::ifstream file(filePath.c_str());
	// if (!file.is_open())
	// 	throw(404);
	// std::stringstream ss;
	// ss << file.rdbuf();
	// _body = ss.str();
	// _contentLength = _body.size();
	// //set status and body
	// _statusCode = 200;
	// //set content type based on the file extension
	// size_t pos = filePath.rfind(".");
	// if (pos != std::string::npos)
	// {
	// 	std::string ext = filePath.substr(pos);
	// 	if (ext == ".html")
	// 		_contentType = "text/html";
	// 	else if (ext == ".css")
	// 		_contentType = "text/css";
	// 	else if (ext == ".js")
	// 		_contentType = "application/javascript";
	// 	else if (ext == ".jpeg")
	// 		_contentType = "image/jpeg";
	// 	else if (ext == ".png")
	// 		_contentType = "image/png";
	// 	else
	// 	 	_contentType = "text/plain"; //it wouldnt try to execute
	// }
	// else
	// {
	// 	//trigger to save/downlaod the file only
	// 	_contentType = "application/octet-stream";
	// 	//throw (404);
	// }


void	Respond::buildResponse()
{
	std::stringstream ss;

	// Status Line: HTTP/1.1 + parsedCode + OK + \r\n
	ss << _protocol << " " << _statusCode << " " << getStatusMsg() << "\r\n";
	// Server Header: Server: Webserv/1.0\r\n
	ss << "Server: " << _serverName << "\r\n";
	// Content-Length: Content-Length: + body.size() + \r\n
	ss << "Content-Length: " << _body.size() << "\r\n";
	// CGI Headers: Add the Content-Type you found earlier.
	std::string ct = (_contentType.empty()) ? "text/html" : _contentType;
	ss << "Content-Type: " << ct << "\r\n";
	std::string cs = (_connStatus == KEEP_ALIVE) ? "keep-alive" : "close";
	ss << "Connection: " << cs << "\r\n";
	// Separator: \r\n
	ss << "\r\n";
	// Body: The actual HTML/data.
	ss << _body;
	_fullResponse = ss.str();
}

std::string Respond::getStatusMsg()
{
	switch(_statusCode)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permenantly";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		case 502: return "Bad Gateway";
		case 504: return "Gateway Timeout";
		default: return "OK";
	}
}

int	Respond::sendResponse()
{	
	const char* dataToSend = _fullResponse.c_str() + _bytesSent;
	size_t		len = _fullResponse.size() - _bytesSent;
	
	ssize_t sent = send( _socketFd, dataToSend, len, 0);
	//_lastActivity update
	if (sent == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; // Not an error! Just wait for the next POLLOUT.
		return -1;
	}
	if (sent == 0)
		return -1;
	//updates byteSent
	_bytesSent += sent;
	if (_bytesSent == _fullResponse.size()) {
		// if (!_client->isKeepAlive()) {
		// 	_client->state = FINISHED;
		// }
		_client->state = FINISHED;
		return 1;
	}
	return 0;
}

void	Respond::buildErrorResponse(int statusCode)
{
	_statusCode = statusCode;

	std::stringstream bodySs;
	if(_body.empty())
	{
		bodySs << "<html>" << "\r\n";
		bodySs << "<head><title>" << _statusCode << "</title></head>";
		bodySs << "<body><center><h1>" << _statusCode << "</h1></center>";
		bodySs << "<hr><center>" << _serverName << "</center></body>";
		bodySs << "</html>";
		_body = bodySs.str();
	}
	std::stringstream ss;

	ss << _protocol << " " << _statusCode << " " << getStatusMsg() << "\r\n";
	ss << "Server: " << _serverName << "\r\n";
	ss << "Content-Type: " << "text/html" << "\r\n";
	ss << "Content-Length: " << _body.size() << "\r\n";
	ss << "Connection: " << "close" << "\r\n";

	ss << "\r\n";
	ss << _body;
	_fullResponse = ss.str();
	_bytesSent = 0;
}

void	Respond::findErrorBody(std::string errorPath)
{
	std::ifstream file(errorPath.c_str());
	if (!file.is_open()) //since i assumed my parser friend already check it
		return ;
	std::stringstream ss; 
	ss << file.rdbuf();
	file.close();
	_body = ss.str();
}

void	Respond::printResponse()
{
	std::cout << "\n==========RESPOND===============" << std::endl;
	std::cout << "Status Code : " << _statusCode << std::endl;
	std::cout << "Protocol: " << _protocol << std::endl;
	std::cout << "Body: " << _body << std::endl;
	std::cout << "Content Type: " << _contentType << std::endl;
	std::cout << "Content Length: " << _contentLength << std::endl;
	std::cout << "Server Name: " << _serverName << std::endl;
	std::cout << "Connection Status: " << _connStatus << std::endl;
	std::cout << "Socket Fd: " << _socketFd;
	std::cout << "\n\nFull Response: \n" << _fullResponse << std::endl;
	std::cout << "\n==========FINISH================" << std::endl;
}

void	Respond::resetResponder()
{
	_fullResponse.clear();
	_statusCode = 0;
	_protocol.clear();
	_body.clear();
	_contentType.clear();
	_contentLength = 0;
	_serverName.clear();
	_connStatus = 0;
	_bytesSent = 0;
}

void Respond::setServerName(const std::string& serverName)
{
	_serverName = serverName;
}

void Respond::setProtocol(const std::string& protocol)
{
	_protocol = protocol;
}

void Respond::handleError(int statusCode)
{	
	_statusCode = statusCode;
	
	if (_protocol.empty()) {
		_protocol = "HTTP/1.1";
	}
	
	std::string errorPath;
	switch(statusCode) {
		case 403:
			errorPath = "./www/errors/403.html";
			break;
		case 404:
			errorPath = "./www/errors/404.html";
			break;
		case 500:
			errorPath = "./www/errors/500.html";
			break;
		default:
			errorPath = "";
			break;
	}

	if (!errorPath.empty()) {
		findErrorBody(errorPath);
	}
	
	if (_body.empty()) {
		std::stringstream bodySs;
		bodySs << "<!DOCTYPE html>\n";
		bodySs << "<html>\n";
		bodySs << "<head><title>" << statusCode << " " << getStatusMsg() << "</title></head>\n";
		bodySs << "<body>\n";
		bodySs << "<center><h1>" << statusCode << " " << getStatusMsg() << "</h1></center>\n";
		bodySs << "<hr><center>" << _serverName << "</center>\n";
		bodySs << "</body>\n";
		bodySs << "</html>\n";
		_body = bodySs.str();
	}
	
	_contentType = "text/html";
	_contentLength = _body.size();
	std::cout << "✅ [ERROR] Error " << statusCode << " handling complete" << std::endl;
}

std::string Respond::getServerRoot() {
	if (_client) {
		return (_client->getRoot());
	}
	return ("./www");
}