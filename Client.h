/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:03:33 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/22 15:12:46 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_H
#define CLIENT_H

#include "CgiRequest.h"
enum e_State {
READ_REQUEST,
EXECUTE_CGI,
WAIT_CGI,
SEND_RESPONSE,
FINISHED,
};

class Client {
private:
	CgiRequest requestor;
public:
	e_State state;
	Client();
	~Client();
};

#endif