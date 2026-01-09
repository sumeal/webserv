/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 10:54:52 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/09 23:59:13 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include "CgiExecute.h"
// #include "CgiRequest.h"
#include "CGI_data.h"
#include "Core.h"
#include <exception>
#include <iostream>

void	cgitest_data(t_location& _location, t_request& _request);
void	test_static_get(t_location& loc, t_request& req);
void 	test_static_post(t_location& loc, t_request& req);
void	test_cgi_get(t_location& loc, t_request& req);
void 	test_cgi_post(t_location& loc, t_request& req);

int main()
{
	// t_data		data;
	t_location	locate;
	t_request	request;

	Core core;
	// cgitest_data(locate, request);
	// test_static_get(locate,  request);
	// test_static_post(locate, request);
	test_cgi_get(locate, request);
	// test_cgi_post(locate, request);
	//parse config FromMuzz
	//create listening socket FromMuzz
	try 
	{
		core.run(locate, request);
	} 
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
	}
	// core.pathCheck("/index.html");
}

void cgitest_data(t_location& loc, t_request& req)
{
    // --- 1. Data from the Request Parser ---
    req.method   = "POST";
    req.uri      = "/cgi-bin/simple.py?user=gemini&id=42"; // The full raw URI
    req.path     = "/cgi-bin/simple.py";                   // Path without query
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

void test_static_get(t_location& loc, t_request& req) {
    req.method = "GET";
    req.path   = "/index.html";   // No ".py", so no CGI
    req.query  = "";              // GET without parameters
    req.body   = "";
    
    loc.cgi_enabled = true;       // Even if enabled, path doesn't match extension
    loc.root        = "./www";
	loc.path          = "/cgi-bin";             // The prefix in the URL
	loc.root          = "./cgi-bin";        // Where scripts are on your disk
}

void test_static_post(t_location& loc, t_request& req) {
    req.method = "POST";
    req.path   = "/notes.txt";  // No "/uploads/" prefix
    req.body   = "Hello!";
    
    loc.root   = "./uploads";           // Root is just the current directory
    loc.cgi_enabled = false;
}

void test_cgi_get(t_location& loc, t_request& req) {
    req.method = "GET";
    req.path   = "/cgi-bin/search.py";
    req.query  = "keyword=42&sort=desc"; // Data is here!
    req.body   = "";                     // GETs usually have no body

    loc.cgi_enabled   = true;
    loc.cgi_extension = ".py";
    loc.root          = "./cgi-bin";
	loc.path          = "/cgi-bin";
    loc.cgi_path      = "/usr/bin/python3";     // Path to the python executable
}

void test_cgi_post(t_location& loc, t_request& req) {
    req.method = "POST";
    req.path   = "/cgi-bin/login.py";
    req.query  = ""; 
    req.body   = "user=student42&pass=password123"; // Data is here!
    req.headers["Content-Length"] = "32";

    loc.cgi_enabled   = true;
    loc.cgi_extension = ".py";
	loc.cgi_path      = "/usr/bin/python3";
	loc.root          = "./cgi-bin";
	loc.path          = "/cgi-bin";
}