#ifndef PARSE_HPP
#define PARSE_HPP

#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <map>
#include "TestServer.hpp"

struct Server;
struct Location;
/*
		Later need to add constructor,destructor....
*/

class Parse
{
	public:
		static int parse_outside(const std::string& line, int current_state);
		static int parse_server(const std::string& line, int current_state, Server& server_config);
		static int parse_location(const std::string& line, int current_state, Location& temp_location);
		static std::string trim_line(const std::string& line);
		static bool is_comment_or_empty(const std::string& line);
		static std::vector<std::string> split_line(const std::string& line);
	private:
};

#endif