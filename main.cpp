/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/10 18:49:37 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"
#include "StateMachine.hpp"
#include "Parser.hpp"

int main(int ac, char *av[])
{
	Context	ctx;

	{
		std::ifstream       file;
		Lexer               lexical_analisys;
		std::vector<Token>  tokens;

		if (ac != 2)
		{
			std::cout << "No valid use. You need to pass a .conf file" << std::endl;
			return (1);
		}
		lexical_analisys.obtainInfile(file, av[1]);
		if (lexical_analisys.tokenVectorization(file))
			return (1);
		tokens = lexical_analisys.getTokens();
		
		for (unsigned long i = 0; i < tokens.size(); i++)
		{
			std::cout << "Token Type: "
					<< tokens[i].typeText
					<< ". "
					<< "Token Value: "
					<< tokens[i].value
					<< " Line number: "
					<< tokens[i].lineNumber
					<< std::endl;
		}
		ctx.tokens = tokens;
	}
	{
		StateMachine	stateMachine(START);
		Parser			parser;
	
		stateMachine.addTransition(START, BALANCE, &Parser::balance);
		stateMachine.addTransition(WORD, KEYWORD, &Parser::keyword);
		stateMachine.addTransition(LBRACET, BEGIN_BLOCK, &Parser::insideBlock);
		stateMachine.addTransition(READING, PARSE_CONTENT, &Parser::readBlockInfo);
		stateMachine.handle(ctx, parser, BALANCE);
		if (ctx.balance == true)
			std::cout << "Sucess!" << std::endl;
		else
		{
			std::cout << "Epic Fail!" << std::endl;
			std::cout << ctx.error << std::endl;
			return (1);
		}
		stateMachine.handle(ctx, parser, KEYWORD);
		if (ctx.blockContext != 0)
		{
			std::cout << "Sucess!" << std::endl;
			std::cout << ctx.blockContext << std::endl;
		}
		stateMachine.handle(ctx, parser, BEGIN_BLOCK);
		if (ctx.bracet == '{')
		{
			std::cout << "Sucess!" << std::endl;
			std::cout << ctx.bracet << std::endl;
			return (0);
		}
		else
		{
			std::cout << "Epic Fail in method bracet!" << std::endl;
			std::cout << ctx.error << std::endl;
			return (1);
		}
	}
}
