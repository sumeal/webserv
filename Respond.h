/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muzz <muzz@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 16:58:34 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/21 11:03:15 by muzz             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPOND_H
#define RESPOND_H

#include <cstddef>
#include <string>

class Client; // Forward declaration

enum e_connectionStatus {
	CLOSE,
	KEEP_ALIVE,
};
//tgk how  muzz  store

class Respond {
private:
    Client* _client;
	std::string	_fullResponse;
	int			_statusCode;
	std::string	_protocol;
	std::string	_body;
	std::string	_contentType;
	int			_contentLength;//
	std::string	_serverName;
	int			_connStatus;//
	//send data
	int			_socketFd;
	size_t		_bytesSent;
public:
	Respond();
	~Respond();
	void	buildErrorResponse(int statusCode);
	void	procCgiOutput(std::string cgiOutput);
	void	procNormalOutput(std::string protocol);
	void	findErrorBody(std::string errorPath);
	int		sendResponse();
	void	buildResponse();
	std::string	getStatusMsg();
	void	printResponse();
	void	resetResponder();
	void	setContentType(const std::string& filePath);
	std::string getRequestPath();
	void	setClient(Client* client);
	void	setSocketFd(int socketFd);
	void	setServerName(const std::string& serverName);
	void	setProtocol(const std::string& protocol);
	void	handleError(int statusCode);  // 🎯 NEW: Central error handler
	std::string getServerRoot();
};

#endif