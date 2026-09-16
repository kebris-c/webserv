/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/16 14:31:09 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"
#include "StateMachine.hpp"
#include "Parser.hpp"

void	transitions(StateMachine& stateMachine)
{
	stateMachine.addTransition(START, BALANCE, &Parser::balance);
	stateMachine.addTransition(BLOCK_KEYWORD, BLOCK_KEYWORD_EVENT, &Parser::blockKeyWord);
	stateMachine.addTransition(LBRACET, BEGIN_BLOCK, &Parser::insideBlock);
	stateMachine.addTransition(RBRACET, CLOSE_BLOCK, &Parser::outsideBlock);
	stateMachine.addTransition(DIRECTIVE, DIRECTIVE_EVENT, &Parser::keyword);	
}

int main(int ac, char *av[])
{
	Context				ctx;
	std::ifstream		file;
	Lexer				lexer;
	ParserState			state;
	ParserEvent			event;
	Parser				parser;
	StateMachine		stateMachine(START);

	ctx.bracet = 0;
	ctx.lineNumber = 0;
	if (ac != 2)
	{
		std::cout << "No valid use. You need to pass a .conf file" << std::endl;
		return (1);
	}
	if (lexer.obtainInfile(file, av[1]))
		return (1);
	if (lexer.checkFileContent(file))
		return (1);
	if (lexer.tokenVectorization(file))
		return (1);
	ctx.tokens = lexer.getTokens();
	transitions(stateMachine);
	state = stateMachine.getCurrentState();
	while (state != END)
	{
		event = stateMachine.getNextEvent(state);
		stateMachine.handle(ctx, parser, event);
		state = stateMachine.getCurrentState();
		if (state == SINTAX_ERROR || state == ERROR)
			break ;
	}
	if (ctx.error != "")
		return (1);
	return (0);
}
