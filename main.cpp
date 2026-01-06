/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 10:54:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/05 23:03:18 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include "CgiExecute.h"
// #include "CgiRequest.h"
#include "CGI_data.h"
#include "Core.h"
#include <exception>
#include <iostream>

void	cgitest_data(t_location& _location, t_request& _request);

int main()
{
	// t_data		data;
	t_location	locate;
	t_request	request;

	Core core;
	cgitest_data(locate, request);

	try 
	{
		core.run(locate, request);
	} 
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
	}
}

void cgitest_data(t_location& loc, t_request& req)
{
    // --- 1. Data from the Request Parser ---
    req.method   = "POST";
    req.uri      = "/cgi-bin/emptybody.py?user=gemini&id=42"; // The full raw URI
    req.path     = "/cgi-bin/emptybody.py";                   // Path without query
    req.query    = "user=gemini&id=42";                 // The query string
    req.version  = "HTTP/1.1";
    
    // Headers must include Content-Length for POST requests
    req.headers["Host"]           = "localhost:8080";
    req.headers["Content-Type"]   = "text/plain";
    req.headers["Content-Length"] = "28";
    req.headers["User-Agent"]     = "Mozilla/5.0";
    req.headers["Accept"]         = "*/*";
    
    req.body = "This is a test body message!";

    // --- 2. Data from the Config Parser (Location block) ---
    loc.path          = "/cgi-bin";             // The prefix in the URL
    loc.root          = "./cgi-bin";        // Where scripts are on your disk
    loc.cgi_enabled   = true;
    loc.cgi_extension = ".py";
    loc.cgi_path      = "/usr/bin/python3";     // Path to the python executable
    
    loc.methods.push_back("GET");
    loc.methods.push_back("POST");

    // --- 3. The "Missing Link" (The Absolute Path) ---
    // In your execute() logic, you need to calculate where the script actually is.
    // Usually: absolute_path = loc.root + (req.path - loc.path)
    // For this test, let's assume it's simply:
    // abs_path = "./www/cgi-bin/test.py"; 
}