/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 17:17:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/12 11:56:47 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../inc/Respond.h"
#include "./../inc/CGI_data.h"
#include "./../inc/Client.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <sys/types.h> //ssize_t
#include <sys/socket.h> //send
#include <iostream>
#include <sys/stat.h>  // For directory operations
#include <dirent.h>    // For directory listing
#include <unistd.h>

//initialize the status code
Respond::Respond(std::map<std::string, std::string>& cookiesMap) : 
	_client(NULL), _sessions(cookiesMap), _statusCode(0),
	_protocol("HTTP/1.1"), _contentLength(0), _serverName("localhost"), 
	_connStatus(KEEP_ALIVE), _socketFd(0), _bytesSent(0)
{}

Respond::~Respond()
{}

//first line check?
// replace throw 502 with return buildErrorResponse(502); if something happens
void	Respond::procCgiOutput(std::string cgiOutput)
{
	if (cgiOutput.empty())
		throw(502);

	//			FIND SEPARATOR(usually \r\n\r\n)
	size_t	separatorPos = cgiOutput.find("\r\n\r\n");
	size_t	offset = 4;
	if (separatorPos == std::string::npos)
	{
		separatorPos = cgiOutput.find("\n\n");
		offset = 2;
	}
	if (separatorPos == std::string::npos)
		throw(502);
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
		size_t	statusDigitPos = header.find_first_of("1234567890", statusPos); //defensive for "Error Code 500 Gateway"
		std::string statusStr = header.substr(statusDigitPos, 3);
		_statusCode = std::atoi(statusStr.c_str());
	}
	if (_statusCode < 100 || _statusCode > 599)
		_statusCode = 502;
	//				FIND CONTENT TYPE
	_contentType = getKeyValue(header, headerLow, "content-type");
	if (_contentType.empty())
		_contentType = "text/html";
	//				FIND LOCATION
	_location = getKeyValue(header, headerLow, "location");
	//				EXTRACT COOKIE
	_setCookie = getKeyValue(header, headerLow, "set-cookie");
	//				EXTRACT BODY	
	_body = cgiOutput.substr(separatorPos + offset);
	_contentLength = _body.length();
	setCurrentTime();
}

std::string	Respond::getKeyValue(std::string &header, std::string &headerLow, std::string key)
{
	size_t pos = headerLow.find(key);
	if (pos == std::string::npos)
		return "";
	size_t start = pos + key.size();
	while (start < header.length() && (header[start] == ' ' 
		|| header[start] == '\t' || header[start] == ':'))
		start++;
	size_t end = header.find("\n", start);
	std::string value;
	if (end == std::string::npos)
		value = header.substr(start);
	else
		value = header.substr(start, end - start);
	if (!value.empty() && value[value.size() - 1] == '\r')
		value.erase(value.size() - 1);
	return value;
}

void Respond::buildNormalCookie()
{
	if (_client->getRequest().cookie.empty())
	{
		std::string	charset	= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		std::string result = "";
		
		while(true)
		{
			result = "";
			for (int i = 0; i < 8; i++)
				result += charset[rand() % charset.size()];
			if (getSession(result).empty())
				break ;
		}
		_setCookie = "_session_id=" + result + "; Max-Age=3600; Path=/";
		std::string greetings[] = {"Hello", "Wassup", "Yow"};
		std::string randomGreet = greetings[rand() % 3];
		
		setSession(result, randomGreet);
	}
}

void	Respond::setSession(std::string key, std::string value)
{
	_sessions[key] = value;
}

std::string	Respond::getSession(std::string key)
{
	std::map<std::string, std::string>::iterator it = _sessions.find(key);

	if (it != _sessions.end())
		return it->second;
	return "";
}

//chatgpt
void Respond::setSocketFd(int socketFd)
{
	_socketFd = socketFd;
}

void Respond::setClient(Client* client)
{
	_client = client;
}

std::string Respond::getRequestPath()
{
	if (_client)
		return (_client->getRequest().path);
	return ("/");
}

void Respond::setContentType(const std::string& filePath)
{
    size_t pos = filePath.rfind(".");
    if (pos != std::string::npos)
	{
		std::string ext = filePath.substr(pos);
        
		if (ext == ".html" || ext == ".htm")
            _contentType = "text/html";
		else if (ext == ".css")
            _contentType = "text/css";
		else if (ext == ".js")
            _contentType = "application/javascript";
		else if (ext == ".json")
            _contentType = "application/json";
		else if (ext == ".png")
            _contentType = "image/png";
		else if (ext == ".jpg" || ext == ".jpeg")
            _contentType = "image/jpeg";
		else if (ext == ".gif")
            _contentType = "image/gif";
		else if (ext == ".txt")
            _contentType = "text/plain";
		else
            _contentType = "application/octet-stream"; // Default binary
	}
    else
        _contentType = "text/plain"; // No extension
}

void	Respond::procNormalOutput(std::string protocol)
{
	std::string requestPath = getRequestPath();
	std::string documentRoot = _client->getBestLocation()->root;
	std::string filePath;
	_protocol = protocol;


	if (requestPath == "/" || requestPath.empty())
		filePath = documentRoot + "/index.html";
	else
		filePath = documentRoot + requestPath;
	
	if (_client->getRequest().method == "GET")
		procGet(filePath);
	else if (_client->getRequest().method == "POST")
		procPost(filePath);
	else if (_client->getRequest().method == "DELETE")
		procDelete(filePath);
	buildNormalCookie();
	setCurrentTime();
}

//handle redirect
//directory. if got index.html show it. if no, check autoindex permission. if yes, do autoindex. if no throw
//handle normal file
void	Respond::procGet(std::string filePath)
{
	//			REDIRECT
	if (_client->getBestLocation()->has_redirect)
	{
		_statusCode = _client->getBestLocation()->redir_status;
		_contentLength = 0;
		return ;
	}
	//			COOKIE
	cookieHandler();
	//				HANDLE DIRECTORY
	std::string requestPath = getRequestPath();
	if (isDirectory(filePath))
	{
		// Try to serve index file
		std::string indexPath = filePath;
		if (!indexPath.empty() && indexPath[indexPath.length() - 1] != '/') 
			indexPath += "/";
		indexPath += "index.html";
		if (access(indexPath.c_str(), F_OK) == 0)
			fileServe(indexPath);
		else
		{
			//			NO INDEXFILE, AUTOINDEX IF PERMISSIBLE
			t_location* location = getCurrentLocation();
			if (location && location->auto_index) 
			{
				_body = generateDirectoryListing(filePath, requestPath);
				_statusCode = 200;
				_contentLength = _body.size();
				_contentType = "text/html";
			}
			//			THROW IF AUTOINDEX IS PROHIBITED
			else
				throw 403;
		}
	}
	//			HANDLE REGULAR FILE	
	else 
		fileServe(filePath);
	if (!_sessionValue.empty() && !_client->getHasCgi())
	{
		std::string greetingHtml = 
			"<div style='position:fixed;top:20px;width:100%;text-align:center;color:white;"
            "font-family:Arial;font-weight:bold;z-index:9999;'> " + _sessionValue + ", Our Fans!</div>";
		_body = _body + greetingHtml;
    	_contentLength = _body.size();
	}
}

void	Respond::cookieHandler()
{
	std::string raw = _client->getRequest().cookie;
	std::string id = "";
	size_t pos = raw.find("_session_id=");
	if (pos != std::string::npos) 
	{
	    id = raw.substr(pos + 12);
	    size_t end = id.find(";");
	    if (end != std::string::npos) 
			id = id.substr(0, end);
	}
	std::string sessionValue = getSession(id);
	if (!id.empty() && sessionValue.empty())
		_client->getRequest().cookie.clear();
	if (!sessionValue.empty())
		_sessionValue = sessionValue;
}

void	Respond::fileServe(std::string filePath)
{
	std::ifstream file(filePath.c_str());
	if (!file.is_open())
	{
		throw 404;
	}	
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	
	_body = buffer.str();
	_statusCode = 200;
	_contentLength = _body.size();
	
	setContentType(filePath);
	setLastModified(filePath);
}

void	Respond::procPost(std::string filePath)
{
	if (isDirectory(filePath))
		throw 405;
	std::ofstream outfile(filePath.c_str(), std::ios::out | std::ios::trunc);
	if (!outfile.is_open())
	{
		throw (500);
	}
	outfile << _client->getRequest().body;
	outfile.close();
	_statusCode = 201;
	_body = "<html><head><title>201 Created</title></head><body>"
			"<h1>File Created Successfully</h1><p>The resource has been uploaded.</p>"
			"<hr><address>Webserv/1.0</address></body></html>";
	_contentLength = _body.size();
	_contentType = "text/html";		
}

void	Respond::procDelete(std::string filePath)
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
		if (errno == ENOENT) 		//File doesnt exist
		{
			throw (404);
		}
		else if (errno == EACCES) 	//Permission denied
		{
			throw (403);
		}
		else						//Other system error
		{
			throw (500);
		}
	}
}

void	Respond::buildResponse()
{
	std::stringstream ss;

	// Status Line: HTTP/1.1 + parsedCode + OK + \r\n
	ss << _protocol << " " << _statusCode << " " << getStatusMsg() << "\r\n";
	ss << "Date: " << _currentTime << "\r\n";
	if (!_lastModified.empty())
		ss << "Last Modified: " << _lastModified << "\r\n";
	// Server Header: Server: Webserv/1.0\r\n
	ss << "Server: " << _serverName << "\r\n";
	// Content-Length: Content-Length: + body.size() + \r\n
	ss << "Content-Length: " << _body.size() << "\r\n";
	// CGI Headers: Add the Content-Type you found earlier.
	if (!_setCookie.empty())
		ss << "Set-Cookie: " << _setCookie << "\r\n";
	std::string ct = (_contentType.empty()) ? "text/html" : _contentType;
	ss << "Content-Type: " << ct << "\r\n";
	if (_statusCode >= 300 && _statusCode < 400) 
	{
		if (_location.empty()) //get location from config
			ss << "Location: " << _client->getBestLocation()->redir_path << "\r\n";
		else //get location from paarsing
			ss << "Location: " << _location << "\r\n";
	}
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
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Request Entity Too Large.";
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
	if (sent == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
		return -1;
	}
	if (sent == 0) //disconnected
		return -1;
	//updates byteSent
	_bytesSent += sent;
	if (_bytesSent == _fullResponse.size())
	{
		_client->state = FINISHED;
		return 1;
	}
	return 0;
}

void	Respond::buildErrorResponse(int statusCode)
{
	_statusCode = statusCode;
	setCurrentTime();
	
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
	
	if (_protocol.empty())
	{
		_protocol = "HTTP/1.1";
	}
	ss << _protocol << " " << _statusCode << " " << getStatusMsg() << "\r\n";
	ss << "Server: " << _serverName << "\r\n";
	ss << "Date: " << _currentTime << "\r\n";
	ss << "Content-Type: " << "text/html" << "\r\n";
	ss << "Content-Length: " << _body.size() << "\r\n";
	ss << "Connection: " << (_connStatus == 1 ? "keep-alive":  "close") << "\r\n";
	// _client->setConnStatus(0);
	ss << "\r\n";
	ss << _body;
	_fullResponse = ss.str();
	_bytesSent = 0;
}

void	Respond::findErrorBody(std::string errorPath)
{
	std::ifstream file(errorPath.c_str());
	if (!file.is_open())
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
	std::cout << "Set-cookie: " << _setCookie << std::endl; 
	// std::cout << "Protocol: " << _protocol << std::endl;
	// std::cout << "Body: " << (_body.length() > 50 ? _body.substr(0, 50) + "..." : _body) << std::endl;
	// std::cout << "Content Length: " << _contentLength << std::endl;
	// std::cout << "Content Type: " << _contentType << std::endl;
	// std::cout << "Date: " << _currentTime << std::endl;
	// if (!_lastModified.empty())
	// 	std::cout << "Last Modified: " << _lastModified << std::endl;
	// std::cout << "Server Name: " << _serverName << std::endl;
	// std::cout << "Connection Status: " << _connStatus << std::endl;
	std::cout << "Socket Fd: " << _socketFd;
	std::cout << "\n\nFull Response: \n" << (_fullResponse.length() > 100 ? _fullResponse.substr(0, 100) + "..." : _fullResponse) << std::endl;
	std::cout << "\n==========FINISH================" << std::endl;
}

//amik data dari struct
//change data from struct to appropriate format
//Sat, 31 Jan 2026 09:30:00 GMT
void	Respond::setCurrentTime()
{
	std::time_t now = std::time(0);
	struct std::tm* gmt = std::gmtime(&now);

	char buffer[100];
	std::strftime(buffer, 100,"%a, %d %b %Y %H:%M:%S GMT", gmt);
	_currentTime = std::string(buffer);
}

void	Respond::setLastModified(const std::string& path)
{
	struct stat fileStat;
	
	if (stat(path.c_str(), &fileStat) != 0)
		return ;
	struct std::tm* gmt = std::gmtime(&fileStat.st_mtime);

	char buffer[100];
	strftime(buffer, sizeof(buffer), "%a,  %d  %b %Y %H:%M:%S GMT", gmt);
	_lastModified = std::string(buffer);
}

void	Respond::resetResponder()
{
	_fullResponse.clear();
	_statusCode = 0;
	_protocol.clear();
	_body.clear();
	_contentType.clear();
	_contentLength = 0;
	_currentTime.clear();
	_serverName.clear();
	_connStatus = 0;
	_filePath.clear();
	_location.clear();
	_currentTime.clear();
	_lastModified.clear();
	_setCookie.clear();
	_sessionValue.clear();
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
	
	if (_protocol.empty())
		_protocol = "HTTP/1.1";
	
	std::string errorPath;
	switch(statusCode)
	{
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

	if (!errorPath.empty())
		findErrorBody(errorPath);
	
	if (_body.empty())
	{
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

std::string Respond::getServerRoot()
{
	if (_client)
		return (_client->getRoot());
	return ("./www");
}

bool Respond::isDirectory(const std::string& path) 
{
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) != 0)
		return false;
	return S_ISDIR(statbuf.st_mode);
}

std::string Respond::generateDirectoryListing(const std::string& dirPath, const std::string& requestPath)
{
	std::ostringstream html;
	
	// HTML header
	html << "<!DOCTYPE html>\n"
		 << "<html><head><title>Index of " << requestPath << "</title>\n"
		 << "<style>\n"
		 << "body { font-family: monospace; margin: 40px; }\n"
		 << "h1 { color: #333; }\n"
		 << "a { text-decoration: none; color: #0066cc; }\n"
		 << "a:hover { text-decoration: underline; }\n"
		 << ".dir { color: #0066cc; font-weight: bold; }\n"
		 << ".file { color: #333; }\n"
		 << "pre { line-height: 1.5; }\n"
		 << "</style></head><body>\n"
		 << "<h1>Index of " << requestPath << "</h1>\n"
		 << "<hr><pre>\n";
	
	// Parent directory link
	if (requestPath != "/" && !requestPath.empty()) 
	{
		std::string parentPath = requestPath;
		if (parentPath[parentPath.length() - 1] == '/')
			parentPath = parentPath.substr(0, parentPath.length() - 1);

		size_t lastSlash = parentPath.find_last_of('/');
		if (lastSlash != std::string::npos)
			parentPath = parentPath.substr(0, lastSlash);

		if (parentPath.empty())
			parentPath = "/";
		html << "<a href=\"" << parentPath << "\" class=\"dir\">[Parent Directory]</a>\n";
	}
	
	// Open directory
	DIR* dir = opendir(dirPath.c_str());
	if (dir == NULL)
		html << "Error: Cannot read directory\n";
	else 
	{
		struct dirent* entry;
		std::vector<std::string> directories;
		std::vector<std::string> files;
		
		// Read directory entries
		while ((entry = readdir(dir)) != NULL) 
		{
			std::string name = entry->d_name;
			
			// Skip hidden files and current directory
			if (name[0] == '.')
				continue;
			
			std::string fullPath = dirPath;
			if (fullPath[fullPath.length() - 1] != '/')
				fullPath += "/";
			fullPath += name;
			
			if (isDirectory(fullPath))
				directories.push_back(name);
			else
				files.push_back(name);
		}
		closedir(dir);
		
		// Sort entries (C++98 compliant)
		std::sort(directories.begin(), directories.end());
		std::sort(files.begin(), files.end());
		
		// Display directories first
		for (std::vector<std::string>::iterator it = directories.begin(); 
				it != directories.end(); ++it) 
		{
			std::string linkPath = requestPath;
			if (linkPath[linkPath.length() - 1] != '/')
				linkPath += "/";
			linkPath += *it + "/";
			html << "<a href=\"" << linkPath << "\" class=\"dir\">" 
				 << *it << "/</a>\n";
		}
		
		// Display files
		for (std::vector<std::string>::iterator it = files.begin(); 
			 it != files.end(); ++it) 
		{
			std::string linkPath = requestPath;
			if (linkPath[linkPath.length() - 1] != '/')
				linkPath += "/";
			linkPath += *it;
			html << "<a href=\"" << linkPath << "\" class=\"file\">" 
				 << *it << "</a>\n";
		}
	}
	
	// HTML footer
	html << "</pre><hr>\n" << "<address>Webserv/1.0</address>\n"
		 << "</body></html>\n";
	
	return html.str();
}

t_location* Respond::getCurrentLocation() 
{
	if (!_client)
		return NULL;
	
	std::string requestPath = getRequestPath();
	t_server serverConfig = _client->getServerConfig();
	t_location* bestMatch = NULL;
	size_t bestMatchLength = 0;
	
	// Find the longest matching location
	for (std::vector<t_location>::iterator it = serverConfig.locations.begin();
		 it != serverConfig.locations.end(); ++it) {
		
		std::string locationPath = it->path;
		
		// Check if request path starts with location path
		if (requestPath.find(locationPath) == 0) 
		{
			if (locationPath.length() > bestMatchLength) 
			{
				bestMatch = &(*it);
				bestMatchLength = locationPath.length();
			}
		}
	}
	
	return bestMatch;
}