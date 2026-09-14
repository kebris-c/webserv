/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:44:51 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/14 21:24:20 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"

void	setError(std::string message, Context& ctx, Parser& parser, ParserState state)
{
	int	lineNumber;

	lineNumber = parser.getTokenIndex();
	ctx.error = message +  " -> " + ctx.tokens[lineNumber].value;
	ctx.lineNumber = lineNumber;
	ctx.state = state;
}

bool	isValidPort(std::string port, Context& ctx, Parser& parser)
{
	int	number;

	for (std::string::size_type i = 0; i < port.size(); ++i)
	{
		if (!std::isdigit(port[i]))
			return (setError("PORT: no valid data type",
				ctx, parser, SINTAX_ERROR), false);
	}
	number = std::atoi(port.c_str());
	if (number < 0 || number > 65535)
		return (setError("PORT: value must be between 0 & 65535",
			ctx, parser, SINTAX_ERROR), false);
	return (true);
}

bool	isValidIP(std::string ip, Context& ctx, Parser& parser)
{
	std::stringstream	ss(ip);
	std::string			octect;
	int					number;
	int					count = 0;

	while (std::getline(ss, octect, '.'))
	{
		if (octect.empty())
			return (setError("IP: no octects (.)",
				ctx, parser, SINTAX_ERROR), false);
		for (std::string::size_type i = 0; i < octect.size(); ++i)
		{
			if (!std::isdigit(octect[i]))
				return (setError("IP: does not have a valid data typer",
					ctx, parser, SINTAX_ERROR), false);
		}
		number = std::atoi(octect.c_str());
		if (number < 0 || number > 255)
			return (setError("IP: value must be between 0 & 255",
				ctx, parser, SINTAX_ERROR), false);
		++count;
	}
	return (count == 4);
}

bool	isValidErrorCode(std::string error, Context& ctx, Parser& parser)
{
	std::string::size_type	letter;

	letter = error.find_first_not_of("0123456789");
	if (letter != std::string::npos)
		return (setError("Invalid size unit", ctx, parser, SINTAX_ERROR), false);
	if (error.size() != 3)
		return (setError("Invalid error code", ctx, parser, SINTAX_ERROR), false);
	return (true);
}

bool	isValidHTML(std::string htmlFileName, Context& ctx, Parser& parser)
{
	if (htmlFileName.size() != 4)
		return (setError("HTML: not valid extension format",
			ctx, parser, SINTAX_ERROR), false);
	if (htmlFileName == "html")
		return (true);
	return (false);
}

bool	isValidRoot(std::string prefix, Context& ctx, Parser& parser)
{
	std::string::size_type	c;
	std::string				word;

	c = prefix.find_first_of("/");
	word = prefix.substr(0, c);
	if (word.size() != 3)
		return (setError("ROOT: format of root value not valid",
			ctx, parser, SINTAX_ERROR), false);
	if (word == "www")
		return (true);
	return (false);
}

bool	isValidMethod(std::string method, Context& ctx, Parser& parser)
{
	for (unsigned int i = 0; i < method.size(); i++)
	{
		if (!isupper(method[i]))
			return (setError("METHOD: "
				+ ctx.currentWord
				+ " must be capitalized",
				ctx, parser, SINTAX_ERROR), false);
	}
	if (method == "GET" || method == "POST" || method == "DELETE")
		return (true);
	else
		return (setError("METHOD: the given method is not valid",
			ctx, parser, SINTAX_ERROR), false);
}

std::string::size_type	isValidClientSize(std::string word, Context& ctx, Parser& parser)
{
	std::string::size_type	measure;

	if (word == ";")
		return (setError("Expected client size",
			ctx, parser, SINTAX_ERROR), 1);
	if (word.empty())
		return (setError("Mising number",
			ctx, parser, SINTAX_ERROR), 1);
	measure = word.find_first_not_of("0123456789");
	if (measure == 0)
		return (setError("Missing number",
			ctx, parser, SINTAX_ERROR), 1);
	if (measure == std::string::npos)
		return (setError("Missing unit",
			ctx, parser, SINTAX_ERROR), 1);
	return (measure);
}

int	checkEndFile(Parser& parser, Context& ctx, ParserState state, std::string message)
{
	unsigned int	i;

	i = parser.getTokenIndex();
	if (i + 1 >= ctx.tokens.size())
		return (setError(message, ctx, parser, state), 1);
	return (0);
}

int	checkNextValue(Parser& parser, Context& ctx, ParserState state, std::string message)
{
	unsigned int	i;

	i = parser.getTokenIndex();
	if (i + 1 >= ctx.tokens.size() || ctx.tokens[i + 1].value == ";")
		return (setError(message, ctx, parser, state), 1);
	return (0);
}

ParserState checkNextElement(Context& ctx, Parser& parser)
{
	int					tokenIndex;
	std::vector<Token>& tokens = ctx.tokens;

	tokenIndex = parser.getTokenIndex();
	if (tokens[tokenIndex + 1].value != ";")
		return (setError("';' is missing",
			ctx, parser, SINTAX_ERROR), ctx.state);
	if (checkEndFile(parser, ctx, END, ""))
		return (ctx.state);
	++tokenIndex;
	if (static_cast<unsigned int>(tokenIndex) + 1 >= tokens.size())
		return (parser.setTokenIndex(tokenIndex), END);
	++tokenIndex;
	if (tokens[tokenIndex].value == "}")
			return (parser.setTokenIndex(tokenIndex), RBRACET);
	if (tokens[tokenIndex].value == "server"
		|| tokens[tokenIndex].value == "location")
		return (parser.setTokenIndex(tokenIndex), BLOCK_KEYWORD);
	parser.setTokenIndex(tokenIndex);
	return (DIRECTIVE);
}
