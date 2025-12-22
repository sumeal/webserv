#include "TestServer.hpp"

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (1);
	}
	std::string config_file = argv[1];
	TestServer t;
	t.parse_config(config_file);
	t.launch();
	return (0);
}