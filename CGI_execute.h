#ifndef CGI_EXECUTE_H
#define CGI_EXECUTE_H

class CGI_execute {
private:
	const t_request&	request;
	const t_location&	locate;
	int		pipe_in[2];
	int		pipe_out[2];
	pid_t	pid;
	std::string	_output;
public:
	CGI_execute(const t_request& request, const t_location& locate);
	~CGI_execute();

	bool	execute();
	bool	isFinished() const;
	void	cleanup();

	std::string&	getOutput() const;
};

#endif