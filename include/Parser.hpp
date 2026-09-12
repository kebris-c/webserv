/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:21 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/12 02:41:44 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
# define PARSER_HPP

# include "Webserv.hpp"
# include "StateMachine.hpp"

struct	ServerConfig
{
	int							port;				/* listen port */
	std::string					host;				/* interface, e.g. 0.0.0.0 or 127.0.0.1 */
	std::string					serverName;			/* optional */
	std::map<int, std::string>	errorPages;			/* status -> file path */
	std::size_t					clientMaxBodySize;	/* bytes */
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

typedef ParserState	(Parser::*ParseAction)(Context&);

class	Parser
{
	private:
		std::vector<std::string>	keyWords;
		std::map<std::string, ParseAction> keywordDispatcher;
		int							tokenIndex;
		ServerConfig				serverContext;
		ParserState					parseListen(Context& ctx);
		ParserState					parseServerName(Context& ctx);
		ParserState					parseClienteSize(Context& ctx);
		ParserState					parseError(Context& ctx);
		ParserState					checkNextElement(Context& ctx);
		void						setError(std::string message, Context& ctx);
		bool						isValidIP(std::string ip, Context& ctx);
		bool						isValidPort(std::string port, Context& ctx);
		std::string::size_type		isValidClientSize(std::string word, Context& ctx);
		
	public:
		Parser();
		~Parser();
		ParserState	balance(Context& ctx);
		ParserState	blockKeyWord(Context& ctx);
		ParserState	insideBlock(Context& ctx);
		ParserState	keyword(Context& ctx);
		ParserState	error(Context& ctx);
		int			getTokenIndex();
};

#endif