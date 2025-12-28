/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_data.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 15:03:57 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/28 14:36:45 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CGI_DATA_H
#define CGI_DATA_H
#include <cstddef>
#include <sched.h>
#include <string>
#include <map>
#include <vector>

typedef struct s_request {
    std::string method;      // GET, POST
    std::string uri;         // /cgi-bin/test.py?x=1
    std::string path;        // /cgi-bin/test.py
    std::string query;       // x=1
    std::string version;     // HTTP/1.1
    std::map<std::string, std::string> headers;
    std::string body;
} t_request;

typedef struct s_location {
    std::string path;              // "/cgi-bin"
    std::string root;              // filesystem root
    std::vector<std::string> methods;
    bool cgi_enabled;
    std::string cgi_extension;     // ".py"
    std::string cgi_path;          // "/usr/bin/python3". the binary
} t_location;

//created at child
//hold by map in core object
//hold by cgiExecute object
// typedef struct s_CGI {
// 	int pipeToCgi;
// 	int pipeFromCgi;
// 	int	clientSocket;
// 	int	exitStatus;
// 	pid_t	pid;
// 	size_t	bodySizeSent;
// 	std::string	output;
// 	bool	writeEnded;
// 	bool	readEnded;
// 	s_CGI();
// } t_CGI;

// inline s_CGI::s_CGI() : pipeToCgi(-1), pipeFromCgi(-1), exitStatus(200), clientSocket(-1), 
// 	pid(-1), bodySizeSent(0), writeEnded(false), readEnded(false) {}

#endif