/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:17 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/13 21:33:37 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"
#include "Utils.hpp"

Parser::Parser()
{
	tokenIndex = 0;
	locationIndex = 0;
	keyWords.push_back("server");
	keyWords.push_back("location");
	keywordDispatcher["listen"] = &Parser::parseListen;
	keywordDispatcher["server_name"] = &Parser::parseServerName;
	keywordDispatcher["client_max_body_size"] = &Parser::parseClientSize;
	keywordDispatcher["error_page"] = &Parser::parseError;
	keywordDispatcher["root"] = &Parser::parseRoot;
	keywordDispatcher["index"] = &Parser::parseIndex;
}

Parser::~Parser()
{}

int	Parser::getTokenIndex()
{
	return (this->tokenIndex);
}

int	Parser::getLocationIndex()
{
	return (this->locationIndex);
}

void	Parser::setTokenIndex(int tokenIndex)
{
	this->tokenIndex = tokenIndex;
}

void	Parser::flushLocationInVector()
{
	serverContext.location.push_back(locationConfig);
}

ParserState	Parser::balance(Context& ctx)
{
	const std::vector<Token>&	tokens = ctx.tokens;
	int							counter = 0;

	for (unsigned int i = 0; i < tokens.size(); i++)
	{
		ctx.currentWord = tokens[i].value;
		if (tokens[i].value == "{")
			counter++;
		else if (tokens[i].value == "}")
		{
			counter--;
			if (counter < 0)
				return (setError("Bracets are not balanced",
					ctx, *this, SINTAX_ERROR), ctx.state);
		}
	}
	return (BLOCK_KEYWORD);
}

ParserState	Parser::blockKeyWord(Context& ctx)
{
	std::vector<Token>& tokens = ctx.tokens;

	if (tokens[tokenIndex].value == "server")
	{
		++tokenIndex;
		return (LBRACET);
	}
	if (tokens[tokenIndex].value == "location")
	{
		++tokenIndex;
		if (tokens[tokenIndex].value[0] == '/')
		{
			locationConfig.path = tokens[tokenIndex].value;
			++tokenIndex;
		}
		return (LBRACET);
	}
	return (setError("No keywords found in the current file",
		ctx, *this, SINTAX_ERROR), ctx.state);
}

ParserState	Parser::insideBlock(Context& ctx)
{
	if (ctx.tokens[tokenIndex].value == "{")
	{
		tokenIndex++;
		return (DIRECTIVE);
	}
	if (ctx.tokens[tokenIndex + 1].value == "}")
	{
		tokenIndex++;
		return (RBRACET);
	}
	--tokenIndex;
	return (setError("Keyword (server or location) must have a bracet format({})",
		ctx, *this, SINTAX_ERROR), ctx.state);
}

ParserState Parser::keyword(Context& ctx)
{
	std::map<std::string, ParseAction>::iterator it;
	it = keywordDispatcher.find(ctx.tokens[tokenIndex].value);

	if (it == keywordDispatcher.end())
		return (setError("Unknown keyword",
			ctx, *this, SINTAX_ERROR), ctx.state);
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	return ((this->*(it->second))(ctx));
}

ParserState	Parser::error(Context& ctx)
{
	(void)ctx;
	return (SINTAX_ERROR);
}

ParserState	Parser::parseListen(Context& ctx)
{
	std::string				ip;
	std::string				port;
	std::string::size_type	colon;

	if (checkEndFile(*this, ctx, SINTAX_ERROR, "No IP:PORT defined"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	colon = ctx.currentWord.find(':');
	if (colon == std::string::npos
		|| colon != ctx.currentWord.rfind(':'))
		return (setError("PORT: (:) does not exist",
			ctx, *this, SINTAX_ERROR), ctx.state);
	ip = ctx.currentWord.substr(0, colon);
	port = ctx.currentWord.substr(colon + 1);
	if (!isValidIP(ip, ctx, *this) || !isValidPort(port, ctx, *this))
		return (ctx.state);
	serverContext.host = ip;
	serverContext.port = std::atoi(port.c_str());
	return (checkNextElement(ctx, *this));
}

ParserState Parser::parseServerName(Context& ctx)
{
	if (checkEndFile(*this, ctx, SINTAX_ERROR, "Servername is not defined"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	serverContext.serverName = ctx.currentWord;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseClientSize(Context& ctx)
{
	int						number;
	char					sizeData;
	std::string::size_type	measure;

	if (checkEndFile(*this, ctx, SINTAX_ERROR, "Expected client size"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	measure = isValidClientSize(ctx.currentWord, ctx, *this);
	if (!measure)
		return (ctx.state);
	number = std::atoi(ctx.currentWord.substr(0, measure).c_str());
	sizeData = ctx.currentWord[measure];
	if (sizeData != 'K' && sizeData != 'M' && sizeData != 'G')
		return (setError("Invalid size unit",
			ctx, *this, SINTAX_ERROR), ctx.state);
	serverContext.clientMaxBodySize = number;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseError(Context& ctx)
{
	std::string::size_type	letter;
	int						errorCode;
	std::string				location;

	if (checkEndFile(*this, ctx, SINTAX_ERROR, "Expected error code"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	letter = ctx.currentWord.find_first_not_of("0123456789");
	if (letter != std::string::npos)
		return (setError("Invalid size unit", ctx, *this, SINTAX_ERROR), ctx.state);
	if (ctx.currentWord.size() != 3)
		return (setError("Invalid error code", ctx, *this, SINTAX_ERROR), ctx.state);
	errorCode = std::atoi(ctx.currentWord.c_str());
	if (errorCode < 300 || errorCode > 599)
		return (setError("Invalid HTTP error code", ctx, *this, SINTAX_ERROR), ctx.state);
	++tokenIndex;
	if (checkEndFile(*this, ctx, SINTAX_ERROR, "Expected error location"))
		return (ctx.state);
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	location = ctx.currentWord;
	serverContext.errorPages[errorCode] = location;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseRoot(Context& ctx)
{
	if (checkEndFile(*this, ctx, SINTAX_ERROR, "Root definition expected"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	if (ctx.currentWord == "www")
		locationConfig.root = ctx.tokens[tokenIndex].value;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseIndex(Context& ctx)
{
	std::string::size_type	dot;

	if (checkEndFile(*this, ctx, SINTAX_ERROR, "Root definition expected"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	dot = ctx.currentWord.find('.');
	if (dot == std::string::npos)
		return (setError("The index does not have html format",
			ctx, *this, SINTAX_ERROR), ctx.state);
	locationConfig.index = ctx.currentWord;
	return (checkNextElement(ctx, *this));
}
