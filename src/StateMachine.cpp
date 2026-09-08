/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StateMachine.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 15:52:43 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/08 19:09:21 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "StateMachine.hpp"

StateMachine::StateMachine()
	:initialState(START), currentState(START)
{};

StateMachine::StateMachine(ParserState initialState)
{
	this->initialState = initialState;
	this->currentState = initialState;
}

StateMachine::StateMachine(const StateMachine& other)
{
	*this = other;
}

void	StateMachine::addTransition(ParserState fromState,
								ParserEvent event,
								ParserState toState,
								ActionFunction function)
{
	TransitionKey	key = std::make_pair(fromState, event);
	Action			action;

	action.nextState = toState;
	action.function = function;
	functions[key] = action;
}

Action	StateMachine::nextTransition(ParserState currentState, ParserEvent event)
{
	TransitionKey	key = std::make_pair(currentState, event);

	std::map<TransitionKey, Action>::iterator it = functions.find(key);
	if (it != functions.end())
	{
		return (it->second);
	}
	else
	{
		std::cerr << "Invalid transition" << std::endl;
		exit(1);
	}
}

void	StateMachine::handle(Context& ctx, Parser& parser, ParserEvent event)
{
	ParserState	currentState = getCurrentState();
	Action	action = nextTransition(currentState, event);

	(parser.*action.function)(ctx);
	setCurrentState(action.nextState);
}

void	StateMachine::setCurrentState(ParserState state)
{
	this->currentState = state;
}

ParserState	StateMachine::getCurrentState()
{
	return (this->currentState);
}
