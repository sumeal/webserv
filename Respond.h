/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 16:58:34 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/31 18:33:16 by mbani-ya         ###   ########.fr       */
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
//tgk how  muzz  store

class Respond {
private:
	t_server&	_server;
    Client* 	_client;
	std::string	_fullResponse;
	int			_statusCode;
	std::string	_protocol;
	std::string	_body;
	std::string	_contentType;
	int			_contentLength;//
	std::string	_serverName;
	int			_connStatus;//
	std::string	_filePath;
	std::string _location;
	std::string	_currentTime;
	std::string	_lastModified;
	//send data
	int			_socketFd;
	size_t		_bytesSent;
public:
	Respond(t_server& serverConf);
	~Respond();
	void	buildErrorResponse(int statusCode);
	void	procCgiOutput(std::string cgiOutput);
	void	procNormalOutput(std::string protocol);
	void	findErrorBody(std::string errorPath);
	void	setContentType(const std::string& filePath);
	int		sendResponse();
	void	buildResponse();
	std::string	getStatusMsg();
	void	printResponse();
	void	resetResponder();
	std::string getRequestPath();
	void	setClient(Client* client);
	void	setSocketFd(int socketFd);
	void	setServerName(const std::string& serverName);
	void	setProtocol(const std::string& protocol);
	void	setCurrentTime();
	void	setLastModified(const std::string& path);
	void	handleError(int statusCode);
	std::string getServerRoot();
	std::string generateDirectoryListing(const std::string& dirPath, const std::string& requestPath);
	bool isDirectory(const std::string& path);
	t_location* getCurrentLocation(); // Get matching location for current request
};

#endif