/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:17 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/15 23:17:15 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"
#include "Utils.hpp"

Parser::Parser()
{
	tokenIndex = 0;
	keyWords.push_back("server");
	keyWords.push_back("location");
	keywordDispatcher["listen"] = &Parser::parseListen;
	keywordDispatcher["server_name"] = &Parser::parseServerName;
	keywordDispatcher["client_max_body_size"] = &Parser::parseClientSize;
	keywordDispatcher["error_page"] = &Parser::parseError;
	keywordDispatcher["root"] = &Parser::parseRoot;
	keywordDispatcher["index"] = &Parser::parseIndex;
	keywordDispatcher["allowed_methods"] = &Parser::parseAllowedMethods;
	keywordDispatcher["autoindex"] = &Parser::parseAutoIndex;
	keywordDispatcher["upload_store"] = &Parser::parseRoot;
	keywordDispatcher["return"] = &Parser::parseReturn;
	keywordDispatcher["cgi_extension"] = &Parser::parseCGI;
	keywordDispatcher["cgi_pass"] = &Parser::parseCGI;
}

Parser::~Parser()
{}

int	Parser::getTokenIndex()
{
	return (this->tokenIndex);
}

void	Parser::setTokenIndex(int tokenIndex)
{
	this->tokenIndex = tokenIndex;
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
			if (i == tokens.size() - 1)
			{
				if (counter != 0)
					return (setError("Bracets are not balanced",
						ctx, *this, SINTAX_ERROR), ctx.state);
			}
		}
	}
	return (BLOCK_KEYWORD);
}

ParserState	Parser::blockKeyWord(Context& ctx)
{
	std::vector<Token>& tokens = ctx.tokens;

	if (tokens[tokenIndex].value == "}")
    {
		if (static_cast<unsigned int>(tokenIndex) + 1 > tokens.size())
			outsideBlock(ctx);
        ++tokenIndex;
        return (RBRACET);
    }
	if (tokens[tokenIndex].value == "server")
	{
		serverContext = ServerConfig();
		++tokenIndex;
		return (LBRACET);
	}
	if (tokens[tokenIndex].value == "location")
	{
		locationConfig = LocationConfig();
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
		++ctx.bracet;
		++tokenIndex;
		return (DIRECTIVE);
	}
	if (ctx.tokens[tokenIndex].value == "}")
	{
		--ctx.bracet;
		++tokenIndex;
		return (RBRACET);
	}
	--tokenIndex;
	return (setError("Keyword (server or location) must have a bracet format({})",
		ctx, *this, SINTAX_ERROR), ctx.state);
}

ParserState	Parser::outsideBlock(Context& ctx)
{
    if (ctx.bracet == 2)
    {
        --ctx.bracet;
        serverContext.location.push_back(locationConfig);
        ++tokenIndex;
        return (BLOCK_KEYWORD);
    }
    if (ctx.bracet == 1)
    {
        --ctx.bracet;
        servers.push_back(serverContext);
		if (ctx.tokens[tokenIndex].value == "server")
			return (BLOCK_KEYWORD);
		if (tokenIndex == static_cast<int>(ctx.tokens.size()))
            return (END);
        ++tokenIndex;
        return (BLOCK_KEYWORD);
    }
    return (setError("Unexpected }", ctx, *this, SINTAX_ERROR), ctx.state);
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

	if (checkNextValue(*this, ctx, SINTAX_ERROR, "No IP:PORT defined"))
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
	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Servername is not defined"))
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

	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Expected client size"))
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

	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Expected error code"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	if (!isValidErrorCode(ctx.currentWord, ctx, *this))
		return (ctx.state);
	errorCode = std::atoi(ctx.currentWord.c_str());
	if (errorCode < 300 || errorCode > 599)
		return (setError("Invalid HTTP error code",
			ctx, *this, SINTAX_ERROR), ctx.state);
	++tokenIndex;
	if (checkEndFile(*this, ctx, SINTAX_ERROR, "Expected error location"))
		return (ctx.state);
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	letter = ctx.currentWord.find('.');
	if (!isValidHTML(ctx.currentWord.substr(letter + 1), ctx, *this))
		return (ctx.state);
	location = ctx.currentWord;
	serverContext.errorPages[errorCode] = location;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseRoot(Context& ctx)
{
	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Root definition expected"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	if (!isValidRoot(ctx.currentWord, ctx, *this))
		return (ctx.state);
	locationConfig.root = ctx.tokens[tokenIndex].value;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseIndex(Context& ctx)
{
	std::string::size_type	dot;
	std::string				htmlFile;

	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Root definition expected"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	dot = ctx.currentWord.find('.');
	if (dot == std::string::npos)
		return (setError("The index does not have html format",
			ctx, *this, SINTAX_ERROR), ctx.state);
	htmlFile = ctx.currentWord.substr(dot + 1);
	if (!isValidHTML(htmlFile, ctx, *this))
		return (ctx.state);
	locationConfig.index = ctx.currentWord;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseAllowedMethods(Context& ctx)
{
	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Method definition expected"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	while (ctx.currentWord != ";")
	{
		if (!isValidMethod(ctx.currentWord, ctx, *this))
			return (ctx.state);
		else
			locationConfig.allowedMethods.push_back(ctx.currentWord);
		ctx.currentWord = ctx.tokens[++tokenIndex].value;
	}
	--tokenIndex;
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseAutoIndex(Context& ctx)
{
	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Autoindex definition expected"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	for (unsigned int i = 0; i < ctx.currentWord.size(); i++)
	{
		if (!islower(ctx.currentWord[i]))
			return (setError("autoindex definition has wrong format",
			ctx, *this, SINTAX_ERROR), ctx.state);
	}
	if (ctx.currentWord == "off" || ctx.currentWord == "on")
	{
		if (ctx.currentWord == "off")
			locationConfig.autoindex = false;
		locationConfig.autoindex = true;
	}
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseReturn(Context& ctx)
{
	int	code;

	if (checkNextValue(*this, ctx, SINTAX_ERROR, "Code and direction expected"))
		return (ctx.state);
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	if (ctx.currentWord.size() != 3)
		return (setError("RETURN: terms must have only three digits",
			ctx, *this, SINTAX_ERROR), ctx.state);
	for (unsigned int i = 0; i < ctx.currentWord.size(); i++)
	{
		if (!isdigit(ctx.currentWord[i]))
			return (setError("RETURN: code must have numbers only",
				ctx, *this, SINTAX_ERROR), ctx.state);
	}
	code = std::atoi(ctx.currentWord.c_str());
	if (code < 300 || code > 599)
		return (setError("RETURN: code value not valid",
			ctx, *this, SINTAX_ERROR), ctx.state);
	locationConfig.redirectCode = code;
	++tokenIndex;
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	if (ctx.currentWord[0] == '/')
		locationConfig.redirect = ctx.currentWord;
	else
		return (setError("RETURN: path is not valid", ctx, *this, SINTAX_ERROR), ctx.state);
	return (checkNextElement(ctx, *this));
}

ParserState	Parser::parseCGI(Context& ctx)
{
	std::string::size_type	separator;
	std::string				criteria;

	if (checkNextValue(*this, ctx, SINTAX_ERROR, "CGI incomplete: extension and pass required"))
		return (ctx.state);
	
	separator = ctx.currentWord.find("_");
	if (!separator)
		return (setError("CGI: the character '_' must be present",
			ctx, *this, SINTAX_ERROR), ctx.state);
	criteria = ctx.currentWord.substr(separator + 1);
	++tokenIndex;
	if (criteria == "extension" && ctx.tokens[tokenIndex].value == ".py")
		locationConfig.cgiExtension = ctx.tokens[tokenIndex].value;
	if (criteria == "pass" && ctx.tokens[tokenIndex].value[0] == '/')
		locationConfig.cgiPass = ctx.tokens[tokenIndex].value;
	return (checkNextElement(ctx, *this));
}
