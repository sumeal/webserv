/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 16:58:34 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/13 15:27:56 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPOND_H
#define RESPOND_H

#include <cstddef>
#include <string>
#include "CGI_data.h"

class Client;

enum e_connectionStatus {
	CLOSE,
	KEEP_ALIVE,
};

class Respond {
private:
    Client* 	_client;
	std::map<std::string, std::string>& _sessions;
	std::string	_fullResponse;
	int			_statusCode;
	std::string	_protocol;
	std::string	_body;
	std::string	_contentType;
	int			_contentLength;
	std::string	_serverName;
	int			_connStatus;
	std::string	_filePath;
	std::string _location;
	std::string	_currentTime;
	std::string	_lastModified;
	std::string	_setCookie;
	std::string	_sessionValue;
	//send data
	int			_socketFd;
	size_t		_bytesSent;
	Respond&	operator=(const Respond& other);
	Respond(Respond& other);
public:
	Respond(std::map<std::string, std::string>& cookiesMap);
	~Respond();
	void		buildErrorResponse(int statusCode);
	void		cookieHandler();
	void		procCgiOutput(std::string cgiOutput);
	std::string	getKeyValue(std::string &header, std::string &headerLow, std::string key);
	void		procNormalOutput(std::string protocol);
	void		procGet(std::string filePath);
	void		procPost(std::string filePath);
	void		procDelete(std::string filePath);
	void		fileServe(std::string filePath);
	void		findErrorBody(std::string errorPath);
	void		setContentType(const std::string& filePath);
	int			sendResponse();
	void		buildResponse();
	std::string	getStatusMsg();
	void		printResponse();
	void		resetResponder();
	std::string getRequestPath();
	void		setClient(Client* client);
	void		setSocketFd(int socketFd);
	void		setServerName(const std::string& serverName);
	void		setProtocol(const std::string& protocol);
	void		setCurrentTime();
	void		setLastModified(const std::string& path);
	void		setSession(std::string key, std::string value);
	void		buildNormalCookie();
	void		handleError(int statusCode);
	std::string getServerRoot();
	std::string generateDirectoryListing(const std::string& dirPath, const std::string& requestPath);
	bool 		isDirectory(const std::string& path);
	t_location* getCurrentLocation(); // Get matching location for current request
	std::string	getSession(std::string key);
};

#endif