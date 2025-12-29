/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Respond.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 16:58:34 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/29 17:22:25 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPOND_H
#define RESPOND_H

#include "Client.h"
#include <cstddef>
#include <string>

class Respond {
private:
	std::string	_fullResponse;
	int			_statusCode;
	std::string	_protocol;
	std::string	_body;
	std::string	_contentType;
	int			_contentLength;//
	std::string	serverName;
	int			_connectionStatus;//
	//send data
	int			_socketFd;
	size_t		_bytesSent;
public:
	Respond();
	~Respond();
	void	buildErrorResponse(int statusCode);
	void	procCgiOutput(std::string cgiOutput);
	int		sendResponse();
	void	buildResponse();
	std::string	getStatusMsg();
};

#endif