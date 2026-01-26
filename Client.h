/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abin-moh <abin-moh@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:03:33 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/26 14:24:12 by abin-moh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_TIMEOUT 5

#include "CGI_data.h"
// #include "CgiRequest.h"
#include <ctime>

class CgiExecute;
class Respond;

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
	s_HttpRequest request;
	Respond*		_responder;
	int			_socket; //FromMuzz
	bool		_hasCgi;
	time_t		_lastActivity;
	int			_connStatus;
	t_server	_serverConfig;//do we need to hold as reference or as copy per client?
	//loop thingy
public:
	e_State state;
	Client(t_server server_config);
	bool		revived; //testing
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
	bool	getHasCgi();
	void	setHasCgi(bool status);
	bool	isIdle(time_t now);
	bool	isKeepAlive();
    void	setConnStatus(bool status);

	
	std::string 	readRawRequest();
	s_HttpRequest& 	getRequest();
	std::string 	getRoot();
	t_server		getServerConfig();
	t_location&		getCgiLocation();
	
	bool	isCGI(const t_request& request, const t_location& locate) const; // check
	bool	isCGIextOK(const t_request& request, const t_location& locate) const; //check
	// CgiRequest* GetCgiReq();
	// t_CGI*	getCgi();
	// void	setCgi(t_CGI* cgi);
};

#endif