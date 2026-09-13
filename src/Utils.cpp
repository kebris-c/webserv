/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:44:51 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/13 20:03:57 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"

void	setError(std::string message, Context& ctx, Parser& parser, ParserState state)
{
	ctx.error = message;
	ctx.lineNumber = parser.getTokenIndex();
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
	if (static_cast<unsigned int>(parser.getTokenIndex()) + 1 >= ctx.tokens.size())
		return (setError(message, ctx, parser, state), 1);
	return (0);
}
