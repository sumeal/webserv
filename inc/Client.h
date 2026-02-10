/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:03:33 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/10 12:41:16 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_TIMEOUT 5

#include "CGI_data.h"
#include <ctime>
#include <cstdlib>
#include "Respond.h"

class CgiExecute;

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
	t_server&	_serverConfig;
	CgiExecute* _executor;
	s_HttpRequest request;
	Respond		_responder;
	int			_socket;
	bool		_hasCgi;
	time_t		_lastActivity;
	int			_connStatus;
	t_location*	_bestLocation;
	std::string	_rawBuffer;
	std::string	_currentRequest;
	bool		_headersParsed;
	size_t		_expectedBodyLength;
	size_t		_currentBodyLength;
	bool		_requestComplete;
	bool		_disconnected;

	bool		_isMaxPayload;
	bool		_isChunked;
	bool		_chunkedComplete;
	std::string	_chunkedBody;
	size_t		_maxBodySize;
	size_t		parseContentLength(const std::string& headers);
	bool		parseTransferEncoding(const std::string& headers);
	bool		isChunkedComplete();
	Client(const Client& other);
	Client&		operator=(const Client& other);
public:
	e_State state;
	Client(t_server& server_config, std::map<std::string, std::string>& cookies);
	~Client();
	void	procInput(int i, struct pollfd& pFd);
	void	procOutput(int i, struct pollfd& pFd);
	void	fdPreCleanup(struct pollfd& pFd);
	void	resetClient();
	int		getSocket();
	void	setSocket(int socket); //askmuzz
	CgiExecute* GetCgiExec();
	void	setCgiExec(CgiExecute* executor);
	Respond&	getRespond();
	bool	isCgiExecuted();
	void	setHasCgi(bool status);
	bool	getHasCgi();
	bool	isIdle(time_t now);
	int		isKeepAlive();
    void	setConnStatus(bool status);
	time_t	getLastActivity();
	void	setLastActivity();
	void	checkBestLocation();

	bool			readHttpRequest();
	bool			isRequestComplete();
	bool			isDisconnected();
	std::string		getCompleteRequest();
	void			resetRequestBuffer();
	
	std::string 	readRawRequest();
	s_HttpRequest& 	getRequest();
	std::string 	getRoot();
	t_server&		getServerConfig();
	void	setMaxBodySize(size_t maxSize);
	bool	isCGI(const s_HttpRequest& request, const t_location& locate) const;
	bool	isCGIextOK(const s_HttpRequest& request, const t_location& locate) const;
	t_location*		getBestLocation();
};

#endif