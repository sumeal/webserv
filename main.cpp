/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 10:54:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/16 16:54:09 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI_execute.h"
#include "CGI_request.h"

int main()
{
	t_location	locate;
	t_request	request;
	CGI_request requestor(request, locate);

	if (!requestor.isCGI())
		return 1;
	executeCGI();
}