/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 16:58:34 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/09 16:43:55 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPOND_H
#define RESPOND_H

#include <cstddef>
#include <string>
#include "CGI_data.h"

enum e_connectionStatus {
	CLOSE,
	KEEP_ALIVE,
};
//tgk how  muzz  store

class Respond {
private:
	std::string	_fullResponse;
	int			_statusCode;
	std::string	_protocol;
	std::string	_body;
	std::string	_contentType;
	int			_contentLength;//
	std::string	_serverName;
	int			_connStatus;//
	std::string	_filePath;
	//send data
	int			_socketFd;
	size_t		_bytesSent;
public:
	Respond();
	~Respond();
	void	buildErrorResponse(int statusCode);
	void	procCgiOutput(std::string cgiOutput);
	void	procNormalOutput(const t_request& request, const t_location& locate);
	void	findErrorBody(std::string errorPath);
	void	setContentType();
	int		sendResponse();
	void	buildResponse();
	std::string	getStatusMsg();
	void	printResponse();
	void	resetResponder();
};

#endif