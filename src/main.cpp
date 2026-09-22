/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/22 19:42:28 by kmarrero         ###   ########.fr       */
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
	
	while (ctx.state != REQ_LINE)
	{
		request.feed("GET /index.html HTTP/1.1\r\n", ctx);
		request.feed("\r\n", ctx);
		request.feed("hello", ctx);
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
