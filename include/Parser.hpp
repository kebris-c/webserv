/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:21 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 17:46:32 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
# define PARSER_HPP

# include "Webserv.hpp"
# include "StateMachine.hpp"

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

struct	ServerConfig
{
	int							port;				/* listen port */
	std::string					host;				/* interface, e.g. 0.0.0.0 or 127.0.0.1 */
	std::string					serverName;			/* optional */
	std::size_t					clientMaxBodySize;	/* bytes */
	std::map<int, std::string>	errorPages;			/* status -> file path */
	std::vector<LocationConfig>	location;
};

typedef ParserState	(Parser::*ParseAction)(ParserContext&);

class	Parser
{
	private:
		std::vector<ServerConfig>			servers;
		std::vector<std::string>			keyWords;
		std::map<std::string, ParseAction>	keywordDispatcher;
		ServerConfig						serverContext;
		LocationConfig						locationConfig;
		int									tokenIndex;
		ParserState							parseRoot(ParserContext& ctx);
		ParserState							parseError(ParserContext& ctx);
		ParserState							parseIndex(ParserContext& ctx);
		ParserState							parseListen(ParserContext& ctx);
		ParserState							parseServerName(ParserContext& ctx);
		ParserState							parseClientSize(ParserContext& ctx);
		ParserState							parseAutoIndex(ParserContext& ctx);
		ParserState							parseAllowedMethods(ParserContext& ctx);
		ParserState							parseReturn(ParserContext& ctx);
		ParserState							parseCGI(ParserContext& ctx);
	public:
		Parser();
		~Parser();
		ParserState	balance(ParserContext& ctx);
		ParserState	blockKeyWord(ParserContext& ctx);
		ParserState	insideBlock(ParserContext& ctx);
		ParserState	outsideBlock(ParserContext& ctx);
		ParserState	keyword(ParserContext& ctx);
		int			getTokenIndex();
		std::vector<ServerConfig> getServer();
		void		setTokenIndex(int index, ParserContext& ctx);
};

#endif