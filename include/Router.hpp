/* **************************************************************************
 * Router.hpp
 * OWNER: kmarrero
 * GOAL: Match request URI to the best LocationConfig (prefix match).
 * LEARN:
 *   - Longest-prefix location matching (nginx-like, no regex required)
 *   - Map URL /kapouet/... to root /tmp/www/... (subject example)
 *   - Decide allowed method / redirect before hitting handlers
 * ************************************************************************** */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Webserv.hpp"
#include "Config.hpp"
#include "Request.hpp"

struct RouteMatch {
	const ServerConfig		*server;
	const LocationConfig	*location;
	std::string				fsPath;		/* resolved filesystem path */
	bool					ok;
};

class Router {
public:
	Router();
	~Router();

	/* Pick server by listen socket / host header later; for now by port index. */
	RouteMatch	match(const ServerConfig &server, const Request &req) const;

private:
	const LocationConfig	*_bestLocation(const ServerConfig &server,
											const std::string &uri) const;
	std::string				_mapToFilesystem(const LocationConfig &loc,
											const std::string &uri) const;
};

#endif /* ROUTER_HPP */
