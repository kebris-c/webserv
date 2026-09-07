/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 18:15:01 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/07 18:43:36 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"

Lexer::Lexer()
{
	specialTokens[';'] = TOKEN_SEMICOLON;
	specialTokens['{'] = TOKEN_LBRACE;
	specialTokens['}'] = TOKEN_RBRACE;
	tokenTypeText[TOKEN_SEMICOLON] = "TOKEN_SEMICOLON";
	tokenTypeText[TOKEN_LBRACE] = "TOKEN_LBRACE";
	tokenTypeText[TOKEN_RBRACE] = "TOKEN_RBRACE";
	tokenTypeText[TOKEN_WORD] = "TOKEN_WORD";
}

Lexer::Lexer(const Lexer& other)
{
	std::cout << "Copy constructor called" << std::endl;
	*this = other;
}

Lexer&	Lexer::operator=(const Lexer& other)
{
	if (this != &other)
	{
		this->tokens = other.tokens;
		this->token = other.token;
	}
	return (*this);
}

Lexer::~Lexer()
{
	std::cout << "Destructor of Lexer called" << std::endl;	
}

void	Lexer::obtainInfile(std::ifstream& file, const std::string& userConfig) const
{
	file.open(userConfig.c_str());
}

void	Lexer::saveInfoInVector()
{
	this->tokens.push_back(this->token);
}

void	Lexer::setToken(TokenType tokenType, std::string& tokenData)
{
	this->token.value = tokenData;
	this->token.type = tokenType;
	this->token.typeText = tokenTypeText.find(tokenType)->second;
	saveInfoInVector();
	tokenData.clear();
}

bool	Lexer::checkSpecialTokens(char c)
{
	return (this->specialTokens.find(c) != this->specialTokens.end());
}

void	Lexer::ignoreComments(std::ifstream& userConfig)
{
	char	c;

	while (userConfig.get(c) && c != '\n')
		;
}

void	Lexer::tokenVectorization(std::ifstream& userConfig)
{
	std::string	word;
	std::string	buffer;

	while (userConfig >> word)
	{
		if (word[0] == '#')
		{
			ignoreComments(userConfig);
			continue ;
		}
		for (std::string::iterator loc = word.begin(); loc != word.end(); loc++)
		{
			if (checkSpecialTokens(*loc))
			{
				if (!buffer.empty())
					setToken(TOKEN_WORD, buffer);
				buffer += *loc;
				setToken(specialTokens.find(*loc)->second, buffer);
			}
			else
				buffer += *loc;
		}
		if (!buffer.empty())
			setToken(TOKEN_WORD, buffer);
	}
	userConfig.close();
}

TokenType	Lexer::getTokenType(int vectorIndex)
{
	return (this->tokens.at(vectorIndex).type);
}

std::string	Lexer::getTokenValue(int vectorIndex)
{
	return (this->tokens.at(vectorIndex).value);
}

std::string	Lexer::getTokenTypeText(int vectorIndex)
{
	return (this->tokens.at(vectorIndex).typeText);
}

const std::vector<Token>&	Lexer::getTokens()
{
	return (this->tokens);
}
