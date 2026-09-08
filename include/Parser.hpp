/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:21 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/08 19:40:21 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
# define PARSER_HPP

# include "Webserv.hpp"
# include "StateMachine.hpp"

struct	ServerConfig
{
	int							port;			/* listen port */
	std::string					host;			/* interface, e.g. 0.0.0.0 or 127.0.0.1 */
	std::string					serverName;		/* optional */
	std::size_t					clientMaxBodySize; /* bytes */
	std::map<int, std::string>	errorPages;		/* status -> file path */
};

struct	LocationConfig
{
	int							redirectCode;	/* 301/302/... */
	bool						autoindex;		/* directory listing on/off */
	std::string					path;			/* URL prefix, e.g. /upload */
	std::string					root;			/* filesystem root for this location */
	std::string					index;			/* default file for directories */
	std::string					cgiPass;		/* interpreter / cgi binary path */
	std::string					redirect;		/* empty = none; else target URL/path */
	std::string					uploadStore;	/* directory for uploads */
	std::string					cgiExtension;	/* e.g. .py */
	std::vector<std::string>	allowedMethods;	/* GET POST DELETE */
};

class	Parser
{
	public:
		int	balance(Context& ctx);
};

#endif