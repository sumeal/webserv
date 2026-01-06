/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:35:03 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/22 13:43:20 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <map>
#include "CGI_data.h"
#include "CgiExecute.h"
#include <poll.h>

class Core {
private:
	// std::map<int, CgiExecute*> _cgiMap;
	std::map<int, Client*> _clients;
	std::vector<struct pollfd> _fds;
	std::vector<struct pollfd> _stagedFds;
	bool	_needCleanup;
public:
	Core();
	~Core();
	void	handleTransition(Client* client);
	void	launchCgi(Client* client, t_location& locate, t_request& request);
	void	cgiRegister(Client* client);
	void	run(t_location& locate, t_request& request);
	//Geminied. muzz part
	void 	serverRegister(int serverFd);
	void 	clientRegister(int clientFd, Client* client);
	void	deleteClient(Client* client);
	void	fdPreCleanup(int fd, int index);
	void	removeFd(int fd);//maybe not needed
	void	respondRegister(Client* client);
	void	handleClientError(Client* client, int statusCode);
	void	addStagedFds();
	void	fdCleanup(); //delStagedFd better naming?

	//testing
	void	acceptMockConnections(t_location& locate, t_request& request, int& clientCount);
	void	forceMockEvents();
};