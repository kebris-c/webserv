/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/11 23:27:02 by kjroydev         ###   ########.fr       */
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
		ParserState		state;
		ParserEvent		event;
	
		stateMachine.addTransition(START, BALANCE, &Parser::balance);
		stateMachine.addTransition(BLOCK_KEYWORD, BLOCK_KEYWORD_EVENT, &Parser::blockKeyWord);
		stateMachine.addTransition(LBRACET, BEGIN_BLOCK, &Parser::insideBlock);
		stateMachine.addTransition(READING, DIRECTIVE_EVENT, &Parser::keyword);
		stateMachine.addTransition(DIRECTIVE, DIRECTIVE_EVENT, &Parser::keyword);
		stateMachine.addTransition(SINTAX_ERROR, END_EVENT, &Parser::error);
		// stateMachine.addTransition(SINTAX_ERROR, END_EVENT, &Parser::)
		while (state != SINTAX_ERROR)
		{
			state = stateMachine.getCurrentState();
			event = stateMachine.getNextEvent(state);
			stateMachine.handle(ctx, parser, event);
			state = stateMachine.getCurrentState();
		}
		if (ctx.error != "")
			return (1);
		return (0);
	}
}
