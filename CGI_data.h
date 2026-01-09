/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_data.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abin-moh <abin-moh@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 15:03:57 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/01/09 15:00:14 by abin-moh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CGI_DATA_H
#define CGI_DATA_H
#include <cstddef>
#include <sched.h>
#include <string>
#include <map>
#include <vector>

typedef struct s_server
{
	std::string server_name;
	int port;
	std::string root;
	std::vector<std::string> index_files;
	std::map<int, std::string> error_pages;
	std::vector<s_location> locations;
} t_server;

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
    bool cgi_enabled;
    //std::vector<std::string> methods;
	//std::string cgi_extension;     // ".py"
    std::string cgi_path;          // "/usr/bin/python3". the binary
	std::vector<std::string> cgi_extension;
	bool allow_get;
	bool allow_post;
	bool allow_delete;
	bool auto_index;
} t_location;


#endif