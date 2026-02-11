/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Helper.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muzz <muzz@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:14:41 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/11 20:33:23 by muzz             ###   ########.fr       */
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
