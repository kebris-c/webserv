#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Webserv.hpp"
#include "Config.hpp"
#include "Request.hpp"

struct RouteMatch
{
	const ServerConfig		*server;
	const LocationConfig	*location;
	std::string				fsPath;		/* resolved filesystem path */
	bool					ok;
};

class	Router
{
	public:
		Router();
		~Router();

		/* Pick server by listen socket / host header later; for now by port index. */
		RouteMatch	match(const ServerConfig &server, const Request &req) const;
		/* Location prefix stripped, then joined to root. Empty if .. or no location. */
		std::string	resolve(const ServerConfig &server, const std::string &uri) const;

	private:
		const LocationConfig	*_bestLocation(const ServerConfig &server,
												const std::string &uri) const;
		std::string				_mapToFilesystem(const LocationConfig &loc,
												const std::string &uri) const;
		bool	isLocationMatch(const std::string& uri, const std::string& location) const;
		bool	isTransversal(const std::string& route) const;
};

#endif /* ROUTER_HPP */
