/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 18:15:01 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/03 18:59:09 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"

Lexer::Lexer()
	:innerPosition(0)
{};

int	Lexer::getI()
{
	return (this->innerPosition);
}

void	Lexer::setI(int i)
{
	this->innerPosition = i;
}

std::ifstream	Lexer::obtainInfile(const std::string &userConfig) const
{
	std::ifstream	file(userConfig);

	if (!file.is_open())
	{
		std::cerr << "Error opening the conf file" << std::endl;
		return ;
	}
	return (file);
}

void	Lexer::ignoreComments(std::ifstream &userInput)
{
	char	c;

	while (userInput.get(c) && c != '\n')
		;
}

void	Lexer::setTokenType(TokenType type)
{
	this->token.type = type;
	
}