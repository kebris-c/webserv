/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParserUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:44:51 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/16 14:28:30 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ParserUtils.hpp"

void	setError(std::string message, Context& ctx, ParserState state)
{
	ctx.error = message +  " -> " + ctx.tokens[ctx.lineNumber].value;
	ctx.state = state;
}

bool	isValidPort(std::string port, Context& ctx)
{
	int	number;

	for (std::string::size_type i = 0; i < port.size(); ++i)
	{
		if (!std::isdigit(port[i]))
			return (setError("PORT: no valid data type",
				ctx, SINTAX_ERROR), false);
	}
	number = std::atoi(port.c_str());
	if (number < 0 || number > 65535)
		return (setError("PORT: value must be between 0 & 65535",
			ctx, SINTAX_ERROR), false);
	return (true);
}

bool	isValidIP(std::string ip, Context& ctx)
{
	std::stringstream	ss(ip);
	std::string			octect;
	int					number;
	int					count = 0;

	while (std::getline(ss, octect, '.'))
	{
		if (octect.empty())
			return (setError("IP: no octects (.)",
				ctx, SINTAX_ERROR), false);
		for (std::string::size_type i = 0; i < octect.size(); ++i)
		{
			if (!std::isdigit(octect[i]))
				return (setError("IP: does not have a valid data typer",
					ctx, SINTAX_ERROR), false);
		}
		number = std::atoi(octect.c_str());
		if (number < 0 || number > 255)
			return (setError("IP: value must be between 0 & 255",
				ctx, SINTAX_ERROR), false);
		++count;
	}
	return (count == 4);
}

bool	isValidErrorCode(std::string error, Context& ctx)
{
	std::string::size_type	letter;

	letter = error.find_first_not_of("0123456789");
	if (letter != std::string::npos)
		return (setError("Invalid size unit", ctx, SINTAX_ERROR), false);
	if (error.size() != 3)
		return (setError("Invalid error code", ctx, SINTAX_ERROR), false);
	return (true);
}

bool	isValidHTML(std::string htmlFileName, Context& ctx)
{
	if (htmlFileName.size() != 4)
		return (setError("HTML: not valid extension format",
			ctx, SINTAX_ERROR), false);
	if (htmlFileName == "html")
		return (true);
	return (false);
}

bool	isValidRoot(std::string prefix, Context& ctx)
{
	std::string::size_type	c;
	std::string				word;

	c = prefix.find_first_of("/");
	word = prefix.substr(0, c);
	if (word.size() != 3)
		return (setError("ROOT: format of root value not valid",
			ctx, SINTAX_ERROR), false);
	if (word == "www")
		return (true);
	return (false);
}

bool	isValidMethod(std::string method, Context& ctx)
{
	for (unsigned int i = 0; i < method.size(); i++)
	{
		if (!isupper(method[i]))
			return (setError("METHOD: "
				+ ctx.currentWord
				+ " must be capitalized",
				ctx, SINTAX_ERROR), false);
	}
	if (method == "GET" || method == "POST" || method == "DELETE")
		return (true);
	else
		return (setError("METHOD: the given method is not valid",
			ctx, SINTAX_ERROR), false);
}

std::string::size_type	isValidClientSize(std::string word, Context& ctx)
{
	std::string::size_type	measure;

	if (word == ";")
		return (setError("Expected client size",
			ctx, SINTAX_ERROR), 1);
	if (word.empty())
		return (setError("Mising number",
			ctx, SINTAX_ERROR), 1);
	measure = word.find_first_not_of("0123456789");
	if (measure == 0)
		return (setError("Missing number",
			ctx, SINTAX_ERROR), 1);
	if (measure == std::string::npos)
		return (setError("Missing unit",
			ctx, SINTAX_ERROR), 1);
	return (measure);
}

int	checkEndFile(Context& ctx, ParserState state, std::string message)
{
	unsigned int	i;

	i = ctx.lineNumber;
	if (i + 1 >= ctx.tokens.size())
		return (setError(message, ctx, state), 1);
	return (0);
}

int	checkNextValue(Context& ctx, ParserState state, std::string message)
{
	unsigned int	i;

	i = ctx.lineNumber;
	if (i + 1 >= ctx.tokens.size() || ctx.tokens[i + 1].value == ";")
		return (setError(message, ctx, state), 1);
	return (0);
}

ParserState checkNextElement(Context& ctx, Parser& parser)
{
	int					tokenIndex;
	std::vector<Token>& tokens = ctx.tokens;

	tokenIndex = parser.getTokenIndex();
	if (tokens[tokenIndex + 1].value != ";")
		return (setError("';' is missing",
			ctx, SINTAX_ERROR), ctx.state);
	if (checkEndFile(ctx, END, ""))
		return (ctx.state);
	++tokenIndex;
	if (static_cast<unsigned int>(tokenIndex) + 1 >= tokens.size())
		return ((parser.setTokenIndex(tokenIndex, ctx)), END);
	++tokenIndex;
	if (tokens[tokenIndex].value == "}")
			return ((parser.setTokenIndex(tokenIndex, ctx)), RBRACET);
	if (tokens[tokenIndex].value == "server"
		|| tokens[tokenIndex].value == "location")
		return ((parser.setTokenIndex(tokenIndex, ctx)), BLOCK_KEYWORD);
	parser.setTokenIndex(tokenIndex, ctx);
	return (DIRECTIVE);
}

int	checkFileExistence(const std::string& path, Context& ctx)
{
	struct stat	fileInfo;

	if (stat(path.c_str(), &fileInfo) == -1)
		return (setError("FILE: does not exist", ctx, ERROR), 1);
	if (!S_ISREG(fileInfo.st_mode))
		return (setError("FILE: is not a file", ctx, ERROR), 1);
	return (0);
}

int	checkDirectoryExistence(const std::string& path, Context& ctx)
{
	struct stat	fileInfo;

	if (stat(path.c_str(), &fileInfo) == -1)
		return (setError("DIRECTORY: does not exist", ctx, ERROR), 1);
	if (!S_ISDIR(fileInfo.st_mode))
		return (setError("DIRECTORY: is not a directory", ctx, ERROR), 1);
	return (0);
}
