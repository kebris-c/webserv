/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 18:15:01 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/08 15:48:04 by kmarrero         ###   ########.fr       */
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

bool	Lexer::wordChecker(const std::string& word)
{
	for (std::string::const_iterator it = word.begin(); it != word.end(); ++it)
	{
		if (!std::isalnum(*it)
		&& (*it != '_'
			&& *it != '/'
			&& *it != '.'
			&& *it != ':'
			&& *it != '-'))
			return (false);
	}
	return (true);
}

int	Lexer::flushWord(std::string& buffer)
{
	if (buffer.empty())
		return (0);
	if (!wordChecker(buffer))
	{
		std::cerr << "The word " << buffer << " has a lexical problem"<< std::endl;
		return (1);
	}
	setToken(TOKEN_WORD, buffer);
	buffer.clear();
	return (0);
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

int	Lexer::tokenVectorization(std::ifstream& userConfig)
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
				if (flushWord(buffer))
					return (1);
				buffer += *loc;
				setToken(specialTokens.find(*loc)->second, buffer);
				buffer.clear();
			}
			else
				buffer += *loc;
		}
		if (flushWord(buffer))
			return (1);
	}
	userConfig.close();
	return (0);
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
