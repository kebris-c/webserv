/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 19:29:14 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/28 19:30:27 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"

Router::Router()
{}

Router::~Router()
{}

RouteMatch	Router::match(const ServerConfig &server, const Request &req) const
{
	RouteMatch	m;

	m.server = &server;
	m.location = _bestLocation(server, req.target());
	m.ok = (m.location != 0);
	if (m.ok)
	{
		m.fsPath = _mapToFilesystem(*m.location, req.target());
		if (m.fsPath.empty())
			m.ok = false;
	}
	return (m);
}

std::string	Router::resolve(const ServerConfig &server, const std::string &uri) const
{
	const LocationConfig	*location;

	location = _bestLocation(server, uri);
	if (location == NULL)
		return ("");
	return (_mapToFilesystem(*location, uri));
}

bool	Router::isLocationMatch(const std::string& uri, const std::string& location) const
{
	if (location == "/")
		return (!uri.empty() && uri[0] == '/');
	if (uri.compare(0, location.size(), location) != 0)
		return (false);
	if (uri.size() == location.size())
		return (true);
	return (uri[location.size()] == '/');
}

const LocationConfig	*Router::_bestLocation(const ServerConfig &server,
												const std::string &uri) const
{
	const LocationConfig	*bestMatch = NULL;

	for (size_t i = 0; i < server.location.size(); ++i)
	{
		if (isLocationMatch(uri, server.location[i].path))
		{
			if (bestMatch == NULL
				|| server.location[i].path.size() > bestMatch->path.size())
				bestMatch = &server.location[i];
		}
	}
	return (bestMatch);
}

bool	Router::isTransversal(const std::string& route) const
{
	size_t						pos;
	size_t						start = 0;
	std::string					element;

	while (start < route.size())
	{
		pos = route.find('/', start);
		if (pos == std::string::npos)
			pos = route.size();
		element = route.substr(start, pos - start);
		if (element == ".." || element == ".")
			return (true);
		start = pos + 1;
	}
	return (false);
}

std::string	Router::_mapToFilesystem(const LocationConfig &loc,
										const std::string &uri) const
{
	std::string	route;

	if (loc.root.empty() || loc.path.size() > uri.size())
		return ("");
	route = uri.substr(loc.path.size());
	if (isTransversal(route))
		return ("");
	while (!route.empty() && route[0] == '/')
		route.erase(0, 1);
	if (route.empty())
		return (loc.root);
	if (!loc.root.empty() && loc.root[loc.root.size() - 1] == '/')
		return (loc.root + route);
	return (loc.root + "/" + route);
}
