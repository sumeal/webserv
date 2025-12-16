/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_request.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/13 23:59:28 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/16 16:26:45 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_REQUEST_H
# define CGI_REQUEST_H

#include "CGI_data.h"

class CGI_request {
private:
	const t_request&	_request;
	const t_location&	_locate;

	CGI_request(const CGI_request& other);
	CGI_request& operator=(const CGI_request& other);
public:
	CGI_request(const t_request& request, const t_location& locate);
	~CGI_request();
	
	bool	isCGI() const;
	bool	isCGIextOK() const;
};

#endif