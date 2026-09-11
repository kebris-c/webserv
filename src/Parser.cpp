/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:17 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/11 18:57:34 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"

Parser::Parser()
{
	tokenIndex = 0;
	keyWords.push_back("server");
	keyWords.push_back("location");
	keywordDispatcher["listen"] = &Parser::parseListen;
	keywordDispatcher["server_name"] = &Parser::parseServerName;
}

Parser::~Parser()
{}

void	Parser::setError(std::string message, Context& ctx)
{
	ctx.error = message;
	ctx.lineNumber = tokenIndex;
}

ParserState	Parser::balance(Context& ctx)
{
	std::vector<Token>	tokens = ctx.tokens;
	int	counter = 0;

	for (unsigned int i = 0; i < tokens.size(); i++)
	{
		ctx.currentWord = tokens[i].value;
		if (tokens[i].value == "{")
			counter++;
		else if (tokens[i].value == "}")
		{
			counter--;
			if (counter < 0)
			{
				ctx.balance = false;
				ctx.error = "File bracet unbalanced";
				return (SINTAX_ERROR);
			}
		}
	}
	ctx.balance = (counter == 0);
	return (WORD);
}

ParserState	Parser::blockKeyWord(Context& ctx)
{
	std::vector<Token>	tokens = ctx.tokens;
	std::string			word;

	for (std::vector<Token>::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		if (it->value != keyWords[tokenIndex])
			continue ;
		else
		{
			tokenIndex++;
			if (it->value == "server")
			{
				ctx.blockContext = SERVER;
				return (LBRACET);
			}
			if (it->value == "location")
			{
				ctx.blockContext = LOCATION;
				return (LBRACET);
			}
		}
	}
	setError("No keywords found in the current file", ctx);
	return (SINTAX_ERROR);
}

ParserState	Parser::checkNextElement(Context& ctx)
{
	unsigned int	i;

	i = tokenIndex + 1;
	if (ctx.tokens[tokenIndex].value == ";")
	{
		if (ctx.tokens[tokenIndex].value == ";"
			&& i < ctx.tokens.size())
		{
			tokenIndex++;
			return (SUCCESS);
		}
		return (SEMICOLON);
	}
	--tokenIndex;
	setError("; is missing", ctx);
	return (SINTAX_ERROR);
}

ParserState	Parser::insideBlock(Context& ctx)
{
	if (ctx.tokens[tokenIndex].value == "{")
	{
		tokenIndex++;
		return (READING);
	}
	if (ctx.state == SEMICOLON && ctx.tokens[tokenIndex].value == "}")
	{
		tokenIndex++;
		return (RBRACET);
	}
	--tokenIndex;
	setError("Keyword (server or location) must a bracet format ({})", ctx);
	return (SINTAX_ERROR);
}

ParserState Parser::keyword(Context& ctx)
{
	std::map<std::string, ParseAction>::iterator it;

	while (true)
	{
		it = keywordDispatcher.find(ctx.tokens[tokenIndex].value);
		if (it == keywordDispatcher.end())
			ctx.state = SINTAX_ERROR;
		ctx.currentWord = ctx.tokens[++tokenIndex].value;
		ctx.state = (this->*(it->second))(ctx);
		if (ctx.state == END)
			break ;
	}
	return (ctx.state);
}

bool	Parser::isValidIP(std::string ip, Context& ctx)
{
	std::stringstream	ss(ip);
	std::string			octect;
	int					number;
	int					count = 0;

	while (std::getline(ss, octect, '.'))
	{
		if (octect.empty())
		{
			setError("IP: no octects (.)", ctx);
			return (false);
		}
		for (std::string::size_type i = 0; i < octect.size(); ++i)
		{
			if (!std::isdigit(octect[i]))
			{
				setError("IP: does not have a valid data type", ctx);
				return (false);
			}
		}
		number = std::atoi(octect.c_str());
		if (number < 0 || number > 255)
		{
			setError("IP: value must be between 0 & 255", ctx);
			return (false);
		}
		++count;
	}
	return (count == 4);
}

bool	Parser::isValidPort(std::string port, Context& ctx)
{
	int	number;

	for (std::string::size_type i = 0; i < port.size(); ++i)
	{
		if (!std::isdigit(port[i]))
		{
			setError("PORT: no valid data type", ctx);
			return (false);
		}
		number = std::atoi(port.c_str());
		if (number < 0 || number > 65535)
		{
			setError("PORT: value must be between 0 & 65535", ctx);
			return (false);
		}
	}
	return (true);
}

ParserState	Parser::parseListen(Context& ctx)
{
	std::string				ip;
	std::string				port;
	std::string::size_type	colon;

	colon = ctx.currentWord.find(':');
	if (colon == std::string::npos
		|| colon != ctx.currentWord.rfind(':'))
	{
		setError("colon (:) does not exist", ctx);
		return (SINTAX_ERROR);
	}
	ip = ctx.currentWord.substr(0, colon);
	port = ctx.currentWord.substr(colon + 1);
	if (!isValidIP(ip, ctx) || !isValidPort(port, ctx))
		return (SINTAX_ERROR);
	serverContext.host = ip;
	serverContext.port = std::atoi(port.c_str());
	tokenIndex++;
	if (!checkNextElement(ctx))
		return (END);
	return (SUCCESS);
}

ParserState	Parser::parseServerName(Context& ctx)
{
	ctx.currentWord = ctx.tokens[tokenIndex].value;
	if (!checkNextElement(ctx))
		return (END);
	serverContext.serverName = ctx.currentWord;
	return (SUCCESS);
}

// ParserState	Parser::parseClienteSize(Context& ctx)
// {
// 	ctx.currentWord = ctx.tokens[tokenIndex].value;
// }
