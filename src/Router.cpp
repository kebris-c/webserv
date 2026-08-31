/* **************************************************************************
 * Router.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

#include "Router.hpp"
#include "Utils.hpp"

Router::Router() {}
Router::~Router() {}

RouteMatch	Router::match(const ServerConfig &server, const Request &req) const
{
	RouteMatch	m;
	m.server = &server;
	m.location = _bestLocation(server, req.target());
	m.ok = (m.location != 0);
	if (m.ok)
		m.fsPath = _mapToFilesystem(*m.location, req.target());
	return (m);
}

const LocationConfig	*Router::_bestLocation(const ServerConfig &server,
												const std::string &uri) const
{
	/*
	 * TODO(kmarrero): longest matching prefix among server.locations
	 * INVESTIGATE: nginx location matching order (prefix vs exact); we only need prefix.
	 */
	(void)server;
	(void)uri;
	return (0);
}

std::string	Router::_mapToFilesystem(const LocationConfig &loc,
										const std::string &uri) const
{
	/*
	 * TODO(kmarrero): subject example:
	 *   location /kapouet -> root /tmp/www
	 *   URI /kapouet/pouic/toto/pouet -> /tmp/www/pouic/toto/pouet
	 * REJECT path traversal ("..").
	 */
	(void)loc;
	(void)uri;
	return ("");
}
