/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_data.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbani-ya <mbani-ya@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 15:03:57 by mbani-ya          #+#    #+#             */
/*   Updated: 2025/12/18 17:21:15 by mbani-ya         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CGI_DATA_H
#define CGI_DATA_H
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

typedef struct s_CGIContext {
	
} t_CGIContext;

#endif