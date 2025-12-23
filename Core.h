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

class Core {
private:
	std::map<int, t_CGI*> _cgi_map;
	std::vector<struct pollfd> _fds;
public:
	Core();
	~Core();
	void	launchCgi(CgiExecute& executor, t_location& locate, t_request& request);
	void	cgiRegister(t_CGI* cgiStruct);
	void	cgiWait(CgiExecute& executor);
	void	run(t_location& locate, t_request& request);
};