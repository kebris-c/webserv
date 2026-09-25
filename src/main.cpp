/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 18:33:33 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"
#include "StateMachine.hpp"
#include "Parser.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Response.hpp"

void	confTransitions(StateMachine& stateMachine)
{
	stateMachine.addTransition(START, BALANCE, &Parser::balance);
	stateMachine.addTransition(BLOCK_KEYWORD, BLOCK_KEYWORD_EVENT, &Parser::blockKeyWord);
	stateMachine.addTransition(LBRACET, BEGIN_BLOCK, &Parser::insideBlock);
	stateMachine.addTransition(RBRACET, CLOSE_BLOCK, &Parser::outsideBlock);
	stateMachine.addTransition(DIRECTIVE, DIRECTIVE_EVENT, &Parser::keyword);
}

void	requestTransitions(RStateMachine& stateMachine)
{
	stateMachine.addTransition(REQ_LINE, REQ_GET_REQUEST, &RequestParser::parseRequestLine);
	stateMachine.addTransition(REQ_HEADERS, REQ_GET_HEADERS, &RequestParser::parseRequestHeader);
	stateMachine.addTransition(REQ_BODY, REQ_GET_BODY, &RequestParser::parseRequestBody);
}

bool	defineServer(char *av, std::vector<ServerConfig>& server)
{
	ParserContext		ctx;
	std::ifstream		file;
	Lexer				lexer;
	ParserState			state;
	ParserEvent			event;
	Parser				parser;
	StateMachine		stateMachine(START);

	ctx.bracet = 0;
	ctx.lineNumber = 0;
	if (lexer.obtainInfile(file, av))
		return (false);
	if (lexer.checkFileContent(file))
		return (false);
	if (lexer.tokenVectorization(file))
		return (false);
	ctx.tokens = lexer.getTokens();
	confTransitions(stateMachine);
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
		return (false);
	server = parser.getServer();
	return (true);
}

bool	defineRequest(const std::string& data)
{
	Request			request;
	RequestParser	parser;
	RequestContext	ctx;
	RStateMachine	stateMachine;
	RequestEvent	event;
	RequestState	state;

	request.feed("GET /index.html HTTP/1.1\r\n", ctx);
	request.feed("Host: localhost:8080\r\n", ctx);
	request.feed("Transfer-Encoding: chunked\r\n", ctx);
	request.feed("\r\n", ctx);
	request.feed("4\r\n", ctx);
	request.feed("Wiki\r\n", ctx);
	request.feed("5\r\n", ctx);
	request.feed("pedia\r\n", ctx);
	request.feed("E\r\n", ctx);
	request.feed(" in\r\n", ctx);
	request.feed("\r\n", ctx);
	request.feed("chunks.\r\n", ctx);
	request.feed("0\r\n", ctx);
	request.feed("\r\n", ctx);
	state = stateMachine.getCurrentState();
	while (state != REQ_COMPLETE)
	{
		event = stateMachine.getNextEvent(state);
		stateMachine.handle(ctx, parser, request, event);
		state = stateMachine.getCurrentState();
		if (state == REQ_ERROR)
			break ;
	}
}

int	main(int ac, char* av[])
{
	std::vector<ServerConfig>	server;
	std::string					data;

	if (ac != 2)
	{
		std::cout << "No valid use. You need to pass a .conf file" << std::endl;
		return (1);
	}
	if (!defineServer(av[1], server))
	{
		std::cerr << "Server could not be defined" << std::endl;
		return (1);
	}
	if (!defineRequest(data) == REQ_ERROR)
	{
		/**
		 * Response	response;
		 * error needs to be treated here
		 */
		std::cerr << "Error 400: Bad request" << std::endl;
		return (1);
	}
	return (0);
}
