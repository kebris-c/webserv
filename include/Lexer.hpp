/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 16:28:00 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/10 13:50:36 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_HPP
# define LEXER_HPP

# include "Webserv.hpp"

enum TokenType
{
	TOKEN_WORD,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_SEMICOLON
};

struct	Token
{
	TokenType	type;
	std::string	value;
	std::string	typeText;
	int			lineNumber;
};

class	Lexer
{
	private:
		Token								token;
		std::vector<Token>					tokens;
		std::map<char, TokenType> 			specialTokens;
		std::map<TokenType, std::string>	tokenTypeText;
		int									lineNumber;
		void								saveInfoInVector();
		bool								checkSpecialTokens(char c);
		int									flushWord(std::string& word);
		bool								wordChecker(const std::string& word);
		void								ignoreComments(std::ifstream& userConfig);
		void								setToken(TokenType tokenType, std::string& tokenData);
	public:
		Lexer();
		~Lexer();
		Lexer(const Lexer& other);
		Lexer&						operator=(const Lexer& other);
		const std::vector<Token>&	getTokens();
		TokenType					getTokenType(int vectorIndex);
		std::string					getTokenValue(int vectorIndex);
		std::string					getTokenTypeText(int vectorIndex);
		int							tokenVectorization(std::ifstream& userConfig);
		void						obtainInfile(std::ifstream& file, const std::string& userConfig) const;
};

#endif