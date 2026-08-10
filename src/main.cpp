/* **************************************************************************
 * main.cpp
 * OWNER: both
 * GOAL: Parse argv, load config, run Server. Clean exit codes. No crashes.
 * LEARN: Subject CLI: ./webserv [configuration file]
 * NOTES: Keep main thin — logic lives in Config/Server.
 * ************************************************************************** */

#include "Webserv.hpp"
#include "Config.hpp"
#include "Server.hpp"

int	main(int argc, char **argv)
{
	std::string	path;

	if (argc > 2)
	{
		std::cerr << "Usage: ./webserv [configuration file]" << std::endl;
		return (1);
	}
	path = (argc == 2) ? argv[1] : WEBSERV_DEFAULT_CONF;

	Config	config;
	/* TODO(kmarrero): implement Config::load — currently stub returns false */
	if (!config.load(path))
	{
		std::cerr << "webserv: failed to load config: " << path << std::endl;
		std::cerr << "hint: kmarrero owns Config parser; see include/Config.hpp"
				  << std::endl;
		return (1);
	}

	Server	server;
	/* TODO(kebris-c): implement Server::configure / Server::run */
	if (!server.configure(config.servers()))
	{
		std::cerr << "webserv: failed to configure server sockets" << std::endl;
		return (1);
	}
	return (server.run());
}
