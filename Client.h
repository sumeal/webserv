/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:03:33 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/03 13:23:03 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
#define CLIENT_H

#include "CGI_data.h"
// #include "CgiRequest.h"
#include "CgiExecute.h"
#include "Respond.h"

enum e_State {
READ_REQUEST,
EXECUTE_CGI,
WAIT_CGI,
SEND_RESPONSE,
FINISHED,
};

class Client {
private:
	CgiExecute* _executor;
	Respond		_responder;
	int			_socket;
	int			_hasCgi;
	// CgiRequest	_requestor;
	// t_CGI*		_cgi;
public:
	e_State state;
	Client();
	~Client();
	int		getSocket();
	void	setSocket(int socket);
	CgiExecute* GetCgiExec();
	void	setCgiExec(CgiExecute* executor);
	Respond&	getRespond();
	bool	isCgiOn();
	void	setHasCgi(bool status);
	// CgiRequest* GetCgiReq();
	// t_CGI*	getCgi();
	// void	setCgi(t_CGI* cgi);
	
};

#endif