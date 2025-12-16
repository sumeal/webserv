/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_request.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 08:44:23 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/16 16:03:06 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI_request.h"
#include "CGI_data.h"
#include <unistd.h>
#include <string>

CGI_request::CGI_request(const t_request& request, const t_location& locate)
	 : _request(request), _locate(locate)
{}

CGI_request::~CGI_request()
{}

//check 2 things
//1. cgi enabled? is the location for script to be run have permission for script
//2. executable name/suffix is correct? and can be execute?
bool	CGI_request::isCGI() const
{
	return (_locate.cgi_enabled && isCGIextOK());
}

//should at least support one. which we focus on .py
bool	CGI_request::isCGIextOK() const
{
	const std::string	path		= _request.path;
	const std::string	cgi_path	= _locate.cgi_path;
	
	//manually without taking from config cgi_ext
	//use rfind to detect the last dot
	size_t lastDot = path.rfind(".");
	if (lastDot == std::string::npos) //how to check npos
		return false; 
	//use path.substr and check
	std::string ext = path.substr(lastDot);
	if (ext != ".cgi" && ext != ".php" && ext != ".py"
		&& ext != ".sh" && ext != ".pl")
		return false;
	
	//use access to check valid or not
	if (access(path.c_str(), R_OK) == -1)
		return false;
	//check interpreter
	if (access(cgi_path.c_str(), X_OK) == -1)
		return false;
	//1 case may need to handle but idk necessary or not.
	//./../../../etc/passwd.
	//but still considering to do this right after location matching or now.
	//bcus if now it might be redundant since static also may need it.
	//this will causes the user to go outside of root & get private info
	//if want to handle, use list and every node is separated by /. 
	//lets say meet .. pop back the last node.
	//then we create a string and compare with "var/www/html"
	return true;
}
