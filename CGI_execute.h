#ifndef CGI_EXECUTE_H
#define CGI_EXECUTE_H

#include "CGI_data.h"

class CGI_execute {
private:
	const t_request&	_request;
	const t_location&	_locate;
	int		pipe_in[2];
	int		pipe_out[2];
	pid_t	pid;
	//for execve. 
    std::string abs_path;    // /var/www/html/cgi-bin/test.py
	//execve.CGI script(eg. py)
    std::string remote_addr; // 192.168.1.5
    int         server_port; // 8080
	std::string	_output;
public:
	CGI_execute(const t_request& request, const t_location& locate);
	~CGI_execute();

	void	preExecute();
	void	execute();
	bool	isFinished() const;
	void	cleanup();

	const std::string&	getOutput() const;
};

#endif