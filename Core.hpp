/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:35:03 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/20 09:07:52 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <map>
#include "CGI_data.h"
#include "CGI_execute.h"

class Core {
private:
	std::map<int, t_CGI*> cgi_map;
	std::vector<struct pollfd> _fds;
public:
	Core();
	~Core();
	void	launchCgi(CGI_execute& executor, t_location& locate, t_request& request);
	void	cgiRegister(t_CGI* cgiStruct);
};