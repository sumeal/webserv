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
	std::map<int, Client*> _clients;
	std::vector<struct pollfd> _fds;
	std::vector<struct pollfd> _stagedFds; //sementara
	std::vector<t_server> server_config;
	std::map<std::string, std::string> _cookies;
	t_server temp_server;
	t_location temp_location;
	std::map<int, size_t> _serverFd;

	std::vector<Client*> activeClients;

	// Server class (location dalam Mad version) FromMuzz
	bool	_needCleanup;
    void parseConnectionHeader(Client* client, const s_HttpRequest& request);
	int new_socket;
	void debugHttpRequest(const t_HttpRequest& request);
    void putIntoCached(s_HttpRequest& request);
	Core(const Core& other);
	Core& operator=(const Core& other);
public:
	Core();
	~Core();
	void	handleTransition(Client* client);
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
	void	pathCheck(std::string path);
	std::map<std::string, std::string>& getCookiesMap();

	//muzz part real
	void	parse_config(const std::string &filename);
	void	parse_http_request(Client* current_client, std::string raw_request);
	void 	initialize_server();
	void 	accepter(int server_fd, size_t server_index);
	void 	clientRegister(int clientFd, Client* client, size_t server_index);



	// void	acceptMockConnections(t_location& locate, t_request& request, int& clientCount);
	void	forceMockEvents();
    void	print_location_config(const t_location& location, int index = 0);
    void	print_all_locations();
};