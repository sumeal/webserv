/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:03:33 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/27 22:26:55 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
#define CLIENT_H

#include "CGI_data.h"
#include "CgiRequest.h"
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
	CgiRequest*	_requestor;
	CgiExecute* _executor;
	t_CGI*		_cgi;
	int			_socket;
public:
	e_State state;
	Client(CgiRequest* requestor);
	~Client();
	int		getSocket();
	void	setSocket(int socket);
	t_CGI*	getCgi();
	void	setCgi(t_CGI* cgi);
	void	setCgiExec(CgiExecute* executor);
	CgiRequest* GetCgiReq();
	CgiExecute* GetCgiExec();
	
};

#endif