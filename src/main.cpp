/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 22:04:38 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"
#include "StateMachine.hpp"
#include "Parser.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Response.hpp"

// void	confTransitions(StateMachine& stateMachine)
// {
// 	stateMachine.addTransition(START, BALANCE, &Parser::balance);
// 	stateMachine.addTransition(BLOCK_KEYWORD, BLOCK_KEYWORD_EVENT, &Parser::blockKeyWord);
// 	stateMachine.addTransition(LBRACET, BEGIN_BLOCK, &Parser::insideBlock);
// 	stateMachine.addTransition(RBRACET, CLOSE_BLOCK, &Parser::outsideBlock);
// 	stateMachine.addTransition(DIRECTIVE, DIRECTIVE_EVENT, &Parser::keyword);
// }

// bool	defineServer(char *av, std::vector<ServerConfig>& server)
// {
// 	ParserContext		ctx;
// 	std::ifstream		file;
// 	Lexer				lexer;
// 	ParserState			state;
// 	ParserEvent			event;
// 	Parser				parser;
// 	StateMachine		stateMachine(START);

// 	ctx.bracet = 0;
// 	ctx.lineNumber = 0;
// 	if (lexer.obtainInfile(file, av))
// 		return (false);
// 	if (lexer.checkFileContent(file))
// 		return (false);
// 	if (lexer.tokenVectorization(file))
// 		return (false);
// 	ctx.tokens = lexer.getTokens();
// 	confTransitions(stateMachine);
// 	state = stateMachine.getCurrentState();
// 	while (state != END)
// 	{
// 		event = stateMachine.getNextEvent(state);
// 		stateMachine.handle(ctx, parser, event);
// 		state = stateMachine.getCurrentState();
// 		if (state == SINTAX_ERROR || state == ERROR)
// 			break ;
// 	}
// 	if (ctx.error != "")
// 		return (false);
// 	server = parser.getServer();
// 	return (true);
// }

// bool	defineRequest(const std::string& data)
// {
// 	Request			request;
// 	RequestParser	parser;
// 	RStateMachine	stateMachine;
// 	RequestEvent	event;
// 	RequestState	state;

// 	state = stateMachine.getCurrentState();
// 	while (state != REQ_COMPLETE)
// 	{
// 		event = stateMachine.getNextEvent(state);
// 		stateMachine.handle(_ctx, parser, request, event);
// 		state = stateMachine.getCurrentState();
// 		if (state == REQ_ERROR)
// 			break ;
// 	}
// }

int	main()
{
	Request			request;
	RequestParser	parser;
	std::vector<std::string>	simulatedFeed;

	// if (ac != 2)
	// {
	// 	std::cout << "No valid use. You need to pass a .conf file" << std::endl;
	// 	return (1);
	// }
	// if (!defineServer(av[1], server))
	// {
	// 	std::cerr << "Server could not be defined" << std::endl;
	// 	return (1);
	// }

	simulatedFeed.push_back("GET /index.html HTTP/1.1\r\n");
	simulatedFeed.push_back("Host: localhost:8080\r\n");
	simulatedFeed.push_back("Transfer-Encoding: chunked\r\n");
	simulatedFeed.push_back("\r\n");
	simulatedFeed.push_back("4\r\n");
	simulatedFeed.push_back("Wiki\r\n");
	simulatedFeed.push_back("5\r\n");
	simulatedFeed.push_back("pedia\r\n");
	simulatedFeed.push_back("E\r\n");
	simulatedFeed.push_back(" in\r\n");
	simulatedFeed.push_back("\r\n");
	simulatedFeed.push_back("chunks.\r\n");
	simulatedFeed.push_back("0\r\n");
	simulatedFeed.push_back("\r\n");

	for (std::vector<std::string>::iterator it = simulatedFeed.begin();
		it != simulatedFeed.end();
		++it)
	{
		std::cout << *it << std::endl;
		if (!request.feed(*it, parser))
		{
			std::cerr << "Error 400: Bad request" << std::endl;
			return (1);
		}
	}
	request.reset();
	if (!request.isComplete())
		return (request.state() != REQ_ERROR);
	return (0);
}
