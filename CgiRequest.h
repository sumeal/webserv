/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiRequest.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/13 23:59:28 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/16 16:26:45 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIREQUEST_H
# define CGIREQUEST_H

#include "CGI_data.h"

class CgiRequest {
private:
	const t_request&	_request;
	const t_location&	_locate;

	CgiRequest(const CgiRequest& other);
	CgiRequest& operator=(const CgiRequest& other);
public:
	CgiRequest(const t_request& request, const t_location& locate);
	~CgiRequest();
	
	bool	isCGI() const;
	bool	isCGIextOK() const;
};

#endif