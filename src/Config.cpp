/* **************************************************************************
 * Config.cpp
 * OWNER: kmarrero
 * GOAL: Implement nginx-inspired parser. Skeleton only.
 * LEARN: Start by parsing the sample configs/default.conf by hand on paper,
 *        then tokenize braces and directives.
 * ************************************************************************** */

#include "Config.hpp"

LocationConfig::LocationConfig()
	: autoindex(false), redirectCode(302), cgiExtension(""), cgiPass("")
{
}

ServerConfig::ServerConfig()
	: host("0.0.0.0"), port(8080), clientMaxBodySize(1048576)
{
}

Config::Config() {}
Config::~Config() {}

bool	Config::load(const std::string &path)
{
	/*
	 * TODO(kmarrero):
	 * 1) open path
	 * 2) strip comments (# ...)
	 * 3) parse server { ... } blocks
	 * 4) fill _servers
	 * INVESTIGATE: nginx listen, root, index, error_page, client_max_body_size,
	 *              limit_except / allowed methods, return, autoindex, cgi
	 */
	(void)path;
	return (false);
}

const std::vector<ServerConfig>	&Config::servers() const
{
	return (_servers);
}
