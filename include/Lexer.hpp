/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 16:28:00 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/03 18:58:42 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_HPP
# define LEXER_HPP

# include "Webserv.hpp";

enum TokenType
{
	TOKEN_WORD,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_SEMICOLON,
	TOKEN_BAR
};

struct	Token
{
	TokenType	type;
	std::string	value;
};

class	Lexer
{
	private:
		std::vector<Token>	tokens;
		Token				token;
		int					innerPosition;
	public:
		Lexer();
		~Lexer();
		TokenType		getTokenType();
		void			setTokenType(TokenType type);
		std::string		getValue();
		void			setValue();
		int				getI();
		void			setI(int i);
		std::ifstream	obtainInfile(const std::string &userConfig) const;
		void			ignoreComments(std::ifstream &userInput);
		void			saveInfoInVector(Token token);
};

#endif