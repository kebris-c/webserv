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

/* Loads configuration, starts all listeners, and runs the server event loop. */
int	main(int argc, char **argv) try
{
	std::string					path;
	std::vector<ServerConfig>	servers;
	bool						echoWorkaround;

	echoWorkaround = false;
	if (argc > 2)
	{
		std::cerr << "Usage: ./webserv [configuration file]" << std::endl;
		return (1);
	}
	path = (argc == 2) ? argv[1] : WEBSERV_DEFAULT_CONF;

	Config	config;
	if (config.load(path))
		servers = config.servers();
	else
	{
		// ============================================================
		// TODO: This is a workaround to allow me keep going on, must be
		// changed/improved before the project end.
		// WHY (kebris-c): Config::load is still a stub; without listeners the
		// I/O plane cannot be developed or defended.
		// YOU (kmarrero): make Config::load(path) succeed for default.conf;
		// then DELETE this entire else-branch and never call
		// enableEchoWorkaround(). Invalid config must return non-zero.
		// RESPECT: keep CLI `./webserv [configuration file]`, thin main,
		// Server::configure(servers) + Server::run() as the only startup path.
		// ============================================================
		ServerConfig	first;
		ServerConfig	second;

		first.host = "127.0.0.1";
		first.port = 8080;
		second.host = "127.0.0.1";
		second.port = 8081;
		servers.push_back(first);
		servers.push_back(second);
		echoWorkaround = true;
		std::cerr << "webserv: temporary echo configuration on ports 8080 and 8081"
				  << std::endl;
	}

	Server	server;
	if (echoWorkaround)
		server.enableEchoWorkaround();
	if (!server.configure(servers))
	{
		std::cerr << "webserv: failed to configure server sockets" << std::endl;
		return (1);
	}
	return (server.run());
}
catch (...)
{
	std::cerr << "webserv: fatal resource or initialization failure" << std::endl;
	return (1);
}
