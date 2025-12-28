/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:03:33 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/28 15:53:13 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
#define CLIENT_H

#include "CGI_data.h"
// #include "CgiRequest.h"
#include "CgiExecute.h"

enum e_State {
READ_REQUEST,
EXECUTE_CGI,
WAIT_CGI,
BUILD_RESPONSE,
SEND_RESPONSE,
FINISHED,
};

class Client {
private:
	CgiExecute* _executor;
	int			_socket;
	// CgiRequest	_requestor;
	// t_CGI*		_cgi;
public:
	e_State state;
	Client();
	~Client();
	int		getSocket();
	void	setSocket(int socket);
	void	setCgiExec(CgiExecute* executor);
	CgiExecute* GetCgiExec();
	// CgiRequest* GetCgiReq();
	// t_CGI*	getCgi();
	// void	setCgi(t_CGI* cgi);
	
};

#endif