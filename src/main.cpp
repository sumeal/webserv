/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 10:54:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/10 16:26:36 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../inc/Core.h"
#include "./../inc/Helper.h"
#include <csignal>
#include <iostream>
#include <csignal>

volatile sig_atomic_t g_shutdown = 0;

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (1);
	}

	std::string config_file = argv[1];
	srand(time(NULL));
	Core core;

	core.parse_config(config_file);
	// core.print_all_locations(); //importantdebug
	core.initialize_server();
	signal(SIGINT, Helper::signalHandler);
	core.run();
	if (g_shutdown == 1)
		core.CleanupAll();
}
