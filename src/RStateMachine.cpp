/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RStateMachine.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 17:07:17 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/22 18:21:20 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RStateMachine.hpp"

RStateMachine::RStateMachine()
	:initialState(REQ_LINE), currentState(REQ_LINE)
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
			return (REQ_READ);
		case REQ_LINE:
			return (REQ_GET_REQUEST);
		default:
			return (END_EVENT);
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
										ActionFunction function)
{
	TransitionKey	key = std::make_pair(fromState, event);
	Action			action;

	action.function = function;
	this->functions[key] = action;
}

Action	RStateMachine::nextTransition(RequestState fromState, RequestEvent event)
{
	TransitionKey	key = std::make_pair(fromState, event);
	Action			invalid;

	std::map<TransitionKey, Action>::iterator it = functions.find(key);
	if (it != functions.end())
		return (it->second);
	else
	{
		std::cerr << "Invalid transition" << std::endl;
		invalid.function = NULL;
		return (invalid);
	}
}

void	RStateMachine::handle(Context& ctx, Request& request, RequestEvent event)
{
	RequestState	answer;
	RequestState	currentState = this->getCurrentState();
	Action			action = this->nextTransition(currentState, event);

	answer = (request.*action.function)(ctx);
	if (answer == REQ_ERROR)
	{
		std::cerr << "In the line: "
		<< ctx.buffer
		<<  " " << ctx.error << std::endl;
		setCurrentState(REQ_ERROR);
	}
	setCurrentState(answer);
}
