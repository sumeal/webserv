/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_data.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abin-moh <abin-moh@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 15:03:57 by mbani-ya          #+#    #+#             */
/*   Updated: 2026/02/05 14:21:41 by abin-moh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CGI_DATA_H
#define CGI_DATA_H
#include <cstddef>
#include <sched.h>
#include <string>
#include <map>
#include <vector>

typedef struct s_location {
    std::string path;              // "/cgi-bin" also for redirection //needcheck
    std::string root;              // filesystem root
    bool cgi_enabled;
	bool has_redirect;
	int  redir_status;
	std::string	redir_path;
   
    std::string interp;          // "/usr/bin/python3". the binary
	std::vector<std::string> cgi_extension;
	bool allow_get;
	bool allow_post;
	bool allow_delete;
	bool auto_index;
	bool location;
	//std::vector<std::string> methods;
	//std::string cgi_extension;     // ".py"
} t_location;

typedef struct s_server
{
	std::string server_name;
	int port;
	std::string root;
	std::vector<std::string> index_files;
	std::map<int, std::string> error_pages;
	std::vector<t_location> locations;
	size_t client_max_body_size;
} t_server;

typedef struct s_HttpRequest
{
    // Request line
    std::string method;          // "GET", "POST", "DELETE"
    std::string uri;             // "/uploads/file.txt?x=1"
    std::string path;            // "/uploads/file.txt"
    std::string query;           // "x=1"
    std::string http_version;    // "HTTP/1.1"
	std::string	cookie;

    // Headers
    std::map<std::string, std::string> headers;

    // Parsed headers (cached for convenience)
    // std::string host;
    // int         port;
    // size_t      content_length;
    // std::string content_type;
    bool        keep_alive;

    std::string body;

    bool is_cgi;
} t_HttpRequest;

#endif