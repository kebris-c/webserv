/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 18:15:01 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/16 12:50:41 by kjroydev         ###   ########.fr       */
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
	lineNumber = 1;
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
{}

int	Lexer::obtainInfile(std::ifstream& file, const std::string& userConfig)
{
	if (userConfig.size() < 5
		|| userConfig.substr(userConfig.size() - 5) != ".conf")
	{
		std::cerr << "Invalid type of file. Must be a .conf" << std::endl;
		return (1);
	}
	file.open(userConfig.c_str());
	if (!file)
	{
		std::cerr << "The given file does not exist" << std::endl;
		return (1);
	}
	file.seekg(0, std::ios::end);
	if (file.tellg() == 0)
	{
		std::cerr << "The configuration file is empty" << std::endl;
		return (1);
	}
	file.seekg(0, std::ios::beg);
	return (0);
}

void	Lexer::ignoreComments(std::ifstream& userConfig)
{
	char	c;

	while (userConfig.get(c) && c != '\n')
		;
}

int		Lexer::checkFileContent(std::ifstream& file)
{
	char	c;

	while (file.get(c))
	{
		if (c == '#')
		{
			ignoreComments(file);
			continue ;
		}
		if (c != ' ' && c != '\n' && c != '\r')
		{
			file.clear();
			file.seekg(0, std::ios::beg);
			return (0);
		}
	}
	file.clear();
	file.seekg(0, std::ios::beg);
	return (1);
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
	this->token.lineNumber = this->lineNumber;
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
		std::cerr << "In line " << this->lineNumber
		<< ": the word " << buffer
		<< " has an invalid character" << std::endl;
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

int	Lexer::tokenVectorization(std::ifstream& userConfig)
{
	std::string	line;
	std::string	buffer;

	while (std::getline(userConfig, line))
	{
		std::string::size_type it = line.find("#");
		if (it != std::string::npos)
		{
			line.erase(it);
			if (line.size() == 0)
			{
				this->lineNumber++;
				continue ;
			}
		}
		for (std::string::iterator loc = line.begin(); loc != line.end(); loc++)
		{
			if (*loc == ' ' || *loc == '\t')
			{
				flushWord(buffer);
				buffer.clear();
			}
			else if (checkSpecialTokens(*loc))
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
		this->lineNumber++;
	}
	userConfig.close();
	return (0);
}
