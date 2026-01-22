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
#include "Parse.hpp"
#include <cctype> 
#include "SocketUtils.hpp"

class Core {
private:
	// std::map<int, CgiExecute*> _cgiMap;
	std::map<int, Client*> _clients;
	std::vector<struct pollfd> _fds;
	std::vector<struct pollfd> _stagedFds; //sementara
	t_server server_config;
	t_location temp_location;
	std::vector<Client*> activeClients;
	// Server class (location dalam Mad version) FromMuzz
	bool	_needCleanup;
	int server_fd;
    void parseConnectionHeader(Client* client, const s_HttpRequest& request);
	int new_socket;


public:
	Core();
	~Core();
	void	handleTransition(Client* client);
	void	launchCgi(Client* client, t_location& locate, t_request& request);
	void	cgiRegister(Client* client);
	void	run();
	//Geminied. muzz part
	void 	serverRegister(int serverFd);
	void	deleteClient(Client* client);
	void	fdPreCleanup(int fd, int index);
	void	removeFd(int fd);//maybe not needed
	void	respondRegister(Client* client);
	void	handleClientError(Client* client, int statusCode);
	void	handleTimeout();
	void	handleSocketDc();
	void	addStagedFds();
	void	fdCleanup(); //delStagedFd better naming?


	//muzz part real
	void	parse_config(const std::string &filename);
	void	parse_http_request(Client* current_client, std::string raw_request);
	void initialize_server();
	void accepter();
	void 	clientRegister(int clientFd, Client* client);



	//testing
	void	acceptMockConnections(t_location& locate, t_request& request, int& clientCount);
	void	forceMockEvents();
    void	print_location_config(const t_location& location, int index = 0);
    void	print_all_locations();
};