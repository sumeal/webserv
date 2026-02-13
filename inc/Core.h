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
#include <iomanip>
#include "SocketUtils.hpp"

class Core {
private:
	std::map<int, Client*> 				_clients;
	std::vector<struct pollfd> 			_fds;
	std::vector<struct pollfd> 			_stagedFds;
	std::vector<t_server> 				server_config;
	std::map<std::string, std::string> 	_cookies;
	t_server 							temp_server;
	t_location 							temp_location;
	std::map<int, size_t> 				_serverFd;
	std::vector<Client*> 				activeClients;
	bool								_needCleanup;
	int 								new_socket;
    void parseConnectionHeader(Client* client, const s_HttpRequest& request);
	void debugHttpRequest(const t_HttpRequest& request);
	Core(const Core& other);
	Core& operator=(const Core& other);
public:
	Core();
	~Core();
	void	handleTransition(Client* client);
	void	cgiRegister(Client* client);
	void	run();
	void 	serverRegister(int serverFd);
	void	deleteClient(Client* client);
	void	fdPreCleanup(int fd);
	void	respondRegister(Client* client);
	void	handleClientError(Client* client, int statusCode);
	void	handleEvents();
	void	handleRequestRead(Client* client);
	void	handleTimeout();
	void	handleSocketDc();
	void	addStagedFds();
	void	fdCleanup(); 
	void	CleanupAll();

	//muzz
	void	parse_config(const std::string &filename);
	void	parse_http_request(Client* current_client, std::string raw_request);
	void 	initialize_server();
	void 	accepter(int server_fd, size_t server_index);
	void 	clientRegister(int clientFd, Client* client, size_t server_index);
	void	forceMockEvents();
    void	print_location_config(const t_location& location, int index = 0);
    void	print_all_locations();
	std::vector<int> _usedPorts;
};