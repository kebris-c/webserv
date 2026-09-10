/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:17 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/10 20:12:42 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"

Parser::Parser()
{
	keyWords.push_back("server");
	keyWords.push_back("location");
	tokenIndex = 0;
	keywordDispatcher["listen"] = &Parser::parseListen;
}

Parser::~Parser()
{}

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
	std::cerr << "No keywords found in the current file" << std::endl;
	return (SINTAX_ERROR);
}

ParserState	Parser::insideBlock(Context& ctx)
{
	if (ctx.tokens[tokenIndex].value == "{")
	{
		tokenIndex++;
		ctx.bracet = ctx.tokens[tokenIndex].value[0];
		return (READING);
	}
	std::cerr << "In line: "
	<< ctx.tokens[tokenIndex].lineNumber
	<< ", there is no bracet"
	<< std::endl;
	return (SINTAX_ERROR);
}

ParserState Parser::keyword(Context& ctx)
{
    std::map<std::string, ParseAction>::iterator it;

    it = keywordDispatcher.find(ctx.tokens[tokenIndex].value);
    if (it == keywordDispatcher.end())
        return (SINTAX_ERROR);
    return (this->*(it->second))(ctx);
}
