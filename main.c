/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 10:54:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/15 16:14:54 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parsing_gpt.h"

int main()
{
	t_location	locate;
	t_request	request;

	if (!isCGI(&locate, &request))
		return 1;
	executeCGI()
}