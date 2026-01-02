#include "Parse.hpp"

int Parse::parse_outside(const std::string& line, int current_state)
{
	(void)current_state; // Unused parameter
    if (line.find("server") != std::string::npos && line.find("{") != std::string::npos) {
        return SERVER;
    }
    return OUTSIDE;
}

int Parse::parse_server(const std::string& line, int current_state, Server& server_config)
{
    std::vector<std::string> tokens = split_line(line);
    
    if (tokens.empty())
		return current_state;
		
	if (tokens[0] == "listen" && tokens.size() >= 2)
		server_config.port = atoi(tokens[1].c_str());
	else if (tokens[0] == "server_name" && tokens.size() >= 2)
		server_config.server_name = tokens[1];
	else if (tokens[0] == "root" && tokens.size() >= 2)
		server_config.root = tokens[1];
	else if (tokens[0] == "index") {
		for (size_t i = 1; i < tokens.size(); i++)  // Start from 1 to skip "index" keyword
			server_config.index_files.push_back(tokens[i]);
	}
	else if (tokens[0] == "error_page" && tokens.size() >= 3) {
		int error_code = atoi(tokens[1].c_str());
		server_config.error_pages[error_code] = tokens[2];
	}
    else if (tokens[0] == "location" && tokens.size() >= 2 && line.find("{") != std::string::npos) {
        return LOCATION;
    }
    else if (line.find("}") != std::string::npos) {
        return OUTSIDE;
    }
    
    return SERVER;
}

int Parse::parse_location(const std::string& line, int current_state, Location& temp_location)
{
	std::vector<std::string> tokens = split_line(line);

	if (tokens.empty())
		return current_state;
	else if (tokens[0] == "allow_methods") {
		for (size_t i = 1; i < tokens.size(); i++) {
			if (tokens[i] == "GET")
				temp_location.allow_get = true;
			else if (tokens[i] == "POST")
				temp_location.allow_post = true;
			else if (tokens[i] == "DELETE")
				temp_location.allow_delete = true;
		}
	}
	else if (tokens[0] == "cgi_ext") {
		for (size_t i = 1; i < tokens.size(); i++) {
			temp_location.cgi_ext.push_back(tokens[i]);
		}
	}
	else if (tokens[0] == "cgi_path" && tokens.size() >= 2) {
		temp_location.cgi_path = tokens[1];
		temp_location.upload_path = "";
	}
	else if (tokens[0] == "upload_path" && tokens.size() >= 2) {
		temp_location.upload_path = tokens[1];
		temp_location.cgi_path = "";
	}
	else if (tokens[0] == "autoindex" && tokens.size() >= 2) {
		temp_location.auto_index = (tokens[1] == "on");
	}
    else if (line.find("}") != std::string::npos) {
        return SERVER;
    }
    return LOCATION;
}

std::string Parse::trim_line(const std::string& line)
{
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    
    size_t end = line.find_last_not_of(" \t\r\n;");
    return line.substr(start, end - start + 1);
}

bool Parse::is_comment_or_empty(const std::string& line)
{
    return line.empty() || line[0] == '#';
}

std::vector<std::string> Parse::split_line(const std::string& line)
{
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}