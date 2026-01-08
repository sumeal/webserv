/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:03:33 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/08 16:06:16 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_TIMEOUT 5

#include "CGI_data.h"
// #include "CgiRequest.h"
#include "CgiExecute.h"
#include "Respond.h"
#include <ctime>

enum e_State {
READ_REQUEST,
HANDLE_REQUEST,
EXECUTE_CGI,
WAIT_CGI,
SEND_RESPONSE,
WAIT_RESPONSE,
FINISHED,
DISCONNECTED,
};

class Client {
private:
	CgiExecute* _executor;
	//HttpRequest	; FromMuzz
	Respond		_responder;
	int			_socket; //FromMuzz
	bool		_hasCgi;
	time_t		_lastActivity;
	int			_connStatus;
	//loop thingy
public:
	e_State state;
	bool		revived; //testing
	Client();
	~Client();
	void	procInput(int i, struct pollfd& pFd);
	void	procOutput(int i, struct pollfd& pFd);
	void	fdPreCleanup(struct pollfd& pFd);
	void	resetClient();
	int		getSocket();
	void	setSocket(int socket);
	CgiExecute* GetCgiExec();
	void	setCgiExec(CgiExecute* executor);
	Respond&	getRespond();
	bool	isCgiOn(); //mcm x perlu
	bool	hasCgi();
	void	setHasCgi(bool status);
	bool	isIdle(time_t now);
	bool	isKeepAlive();
	// CgiRequest* GetCgiReq();
	// t_CGI*	getCgi();
	// void	setCgi(t_CGI* cgi);
};

#endif