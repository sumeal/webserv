#include "TestServer.hpp"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (1);
	}
	std::string config_file = argv[1];
	TestServer t;
	
	// Parse config file (COMPONENT 1)
	t.parse_config(config_file);
	
	// Initialize server with config values
	t.initialize_server();
	
	// Debug: Print parsed config
	t.print_config();
	
	// Launch server with correct port and settings
	t.launch();
	return (0);
}