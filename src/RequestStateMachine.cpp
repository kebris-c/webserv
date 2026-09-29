/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestStateMachine.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 17:07:17 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/29 18:57:55 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestStateMachine.hpp"

RStateMachine::RStateMachine()
	:initialState(REQ_FEED), currentState(REQ_FEED)
{}

RStateMachine::RStateMachine(RequestState initialState)
{
	this->initialState = initialState;
	this->currentState = initialState;
}

RStateMachine::RStateMachine(const RStateMachine& other)
{
	*this = other;
}

RStateMachine::~RStateMachine()
{}

RequestState	RStateMachine::getCurrentState()
{
	return (this->currentState);
}

void			RStateMachine::setCurrentState(RequestState state)
{
	this->currentState = state;
}

RequestEvent	RStateMachine::getNextEvent(RequestState fromState)
{
	switch (fromState)
	{
		case REQ_FEED:
			return (REQ_WAIT_INFO);
		case REQ_LINE:
			return (REQ_GET_REQUEST);
		case REQ_HEADERS:
			return (REQ_GET_HEADERS);
		case REQ_BODY:
			return (REQ_GET_BODY);
		case REQ_WAIT:
			return (REQ_WAIT_INFO);
		default:
			return (REQUEST_END_EVENT);
	}
}

RStateMachine&	RStateMachine::operator=(const RStateMachine& other)
{
	if (this != &other)
	{
		this->initialState = other.initialState;
		this->currentState = other.currentState;
		this->functions = other.functions;
	}
	return (*this);
}

void	RStateMachine::addTransition(RequestState fromState,
										RequestEvent event,
										RequestFunction function)
{
	RequestTransitionKey	key = std::make_pair(fromState, event);
	RequestAction			action;

	action.function = function;
	this->functions[key] = action;
}

RequestAction	RStateMachine::nextTransition(RequestState fromState, RequestEvent event)
{
	RequestTransitionKey	key = std::make_pair(fromState, event);
	RequestAction			invalid;

	std::map<RequestTransitionKey, RequestAction>::iterator it = functions.find(key);
	if (it != functions.end())
		return (it->second);
	else
	{
		invalid.function = NULL;
		return (invalid);
	}
}

void	RStateMachine::handle(RequestContext& ctx, RequestParser& parser, Request& request, RequestEvent event)
{
	RequestState	answer;
	RequestState	currentState = this->getCurrentState();

	if (currentState == REQ_WAIT)
		event = getNextEvent(currentState);

	RequestAction	action = this->nextTransition(currentState, event);

	if (action.function == NULL)
	{
		std::cerr << "Invalid transition" << std::endl;
		setCurrentState(REQ_ERROR);
		return ;
	}
	answer = (parser.*action.function)(ctx, request);
	if (answer == REQ_ERROR)
	{
		std::cerr << "In: " << ctx.buffer << std::endl;
		std::cerr << ctx.error << std::endl;
		setCurrentState(REQ_ERROR);
	}
	setCurrentState(answer);
}
