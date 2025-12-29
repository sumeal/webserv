/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 17:17:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/29 17:30:54 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Respond.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <sstream>
#include <variant>
#include <sys/types.h> //ssize_t
#include <sys/socket.h> //send

Respond::Respond()
{
	
}

Respond::~Respond()
{
	
}

//first line check?
void	Respond::procCgiOutput(std::string cgiOutput)
{
	if (cgiOutput.empty())
	{
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
		buildErrorResponse(502);
		return ;
	}
	//				EXTRACT HEADER		
	std::string	header = cgiOutput.substr(0, separatorPos);
	std::string headerLow = header;
	for (int i = 0; i < headerLow.length(); i++)
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
		_statusCode = 502;
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
}

void	Respond::buildResponse()
{
	std::stringstream ss;

	// How you build it:
	
	// Status Line: HTTP/1.1 + parsedCode + OK + \r\n
	ss << _protocol << " " << _statusCode << " " << getStatusMsg() << "\r\n";
	// Server Header: Server: Webserv/1.0\r\n
	ss << "Server: " << serverName << "\r\n";
	// Content-Length: Content-Length: + body.size() + \r\n
	ss << "Content-Length: " << _body.size() << "\r\n";
	// CGI Headers: Add the Content-Type you found earlier.
	std::string ct = (_contentType.empty()) ? "text/html" : _contentType;
	ss << "Content-Type: " << ct << "\r\n";
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
		case 301: return "Mover Permenantly";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		case 502: return "Bad Gateway";
		default: return "OK";
	}
}

int	Respond::sendResponse()
{	
	const char* dataToSend = _fullResponse.c_str() + _bytesSent;
	size_t		len = _fullResponse.size() - _bytesSent;
	
	ssize_t sent = send( _socketFd, dataToSend, len, 0);
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
	if (_bytesSent == _fullResponse.size())
		return 1;
	return 0;
}

void	Respond::buildErrorResponse(int statusCode)
{
	_statusCode = statusCode;

	std::stringstream bodySs;
	bodySs << "<html>" << "\r\n";
	bodySs << "<head><title>" << _statusCode << "</title></head>";
	bodySs << "<body><center><h1>" << _statusCode << "</h1></center>";
	bodySs << "<hr><center>" << serverName << "</center></body>";
	bodySs << "</html>";
	_body = bodySs.str();
	
	std::stringstream ss;

	ss << _protocol << " " << _statusCode << " " << getStatusMsg() << "\r\n";
	ss << "Server: " << serverName << "\r\n";
	ss << "Content-Type: " << "text/html" << "\r\n";
	ss << "Content-Length: " << _body.size() << "\r\n";
	ss << "Connection: " << "close" << "\r\n";

	ss << "\r\n";
	ss << _body;
	_fullResponse = ss.str();
	_bytesSent = 0;
}
