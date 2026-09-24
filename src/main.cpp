/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/24 16:34:29 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include "Response.hpp"

int	main()
{
	Request	request;
	std::string	data;
	Context	ctx;
	RStateMachine	stateMachine;
	RequestEvent	event;
	RequestState	state;

	stateMachine.addTransition(REQ_LINE, REQ_GET_REQUEST, &Request::requestLine);
	stateMachine.addTransition(REQ_HEADERS, REQ_GET_HEADERS, &Request::requestHeader);
	stateMachine.addTransition(REQ_BODY, REQ_GET_BODY, &Request::requestBody);
	
	while (ctx.state != REQ_LINE)
	{
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
	}
	state = stateMachine.getCurrentState();
	while (state != REQ_COMPLETE)
	{
		event = stateMachine.getNextEvent(state);
		stateMachine.handle(ctx, request, event);
		state = stateMachine.getCurrentState();
		if (state == REQ_ERROR)
			break ;
	}
	if (request.getCurrentState() == REQ_ERROR)
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
