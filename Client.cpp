/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 00:05:01 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/23 13:52:43 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.h"

Client::Client(CgiRequest* requestor) : state(READ_REQUEST),  _requestor(requestor)
{}

Client::~Client()
{}
