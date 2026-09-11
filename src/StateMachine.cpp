/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StateMachine.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 15:52:43 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/11 23:45:19 by kjroydev         ###   ########.fr       */
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

StateMachine::~StateMachine()
{}

void	StateMachine::addTransition(ParserState fromState,
								ParserEvent event,
								ActionFunction function)
{
	TransitionKey	key = std::make_pair(fromState, event);
	Action			action;

	action.function = function;
	functions[key] = action;
}

Action	StateMachine::nextTransition(ParserState currentState, ParserEvent event)
{
	TransitionKey	key = std::make_pair(currentState, event);

	std::map<TransitionKey, Action>::iterator it = functions.find(key);
	if (it != functions.end())
		return (it->second);
	else
	{
		std::cerr << "Invalid transition" << std::endl;
		exit(1);
	}
}

void	StateMachine::handle(Context& ctx, Parser& parser, ParserEvent event)
{
	ParserState	currentState = getCurrentState();
	Action		action = nextTransition(currentState, event);
	ParserState	answer;

	answer = (parser.*action.function)(ctx);
	if (answer == SINTAX_ERROR)
	{
		std::cerr << "In line "
		<< ctx.tokens[ctx.lineNumber].lineNumber
		<< ": " << ctx.error << std::endl;
		setCurrentState(SINTAX_ERROR);
		return ;
	}
	setCurrentState(answer);
}

void	StateMachine::setCurrentState(ParserState state)
{
	this->currentState = state;
}

ParserState	StateMachine::getCurrentState()
{
	return (this->currentState);
}

ParserEvent StateMachine::getNextEvent(ParserState state)
{
	switch (state)
	{
		case START:
			return (BALANCE);
		case BLOCK_KEYWORD:
			return (BLOCK_KEYWORD_EVENT);
		case LBRACET:
			return (BEGIN_BLOCK);
		case READING:
			return (DIRECTIVE_EVENT);
		case DIRECTIVE:
			return (DIRECTIVE_EVENT);
		case SEMICOLON:
			return (CLOSE_BLOCK);
		default:
			return (END_EVENT);
	}
}
