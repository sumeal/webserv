/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Helper.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:14:41 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/10 16:25:36 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../inc/Helper.h"
#include <csignal>

extern volatile sig_atomic_t g_shutdown;

void	Helper::signalHandler(int signum)
{
	(void) signum;
	g_shutdown = 1;
}
