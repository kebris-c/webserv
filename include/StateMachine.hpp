/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StateMachine.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 19:46:24 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/08 19:46:45 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STATEMACHINE_HPP
# define STATEMACHINE_HPP

# include "Webserv.hpp"
# include "Lexer.hpp"

enum ParserState
{
	START,
	WORD,
	RBRACET,
	LBRACET,
	READING,
	SINTAX_ERROR,
	END
};

enum ParserEvent
{
	BALANCE,
	KEYWORD,
	BEGIN_BLOCK,
	CLOSE_BLOCK,
	PARSE_CONTENT,
	EOF_REACHED,
	ERROR
};

struct	Context
{
	std::string			line;
	std::string			error;
	std::string			currentWord;
	int					lineNumber;
	std::vector<Token>	tokens;
	bool				balance;
};

/**
 * @brief Function contained in Parser class. (Consult `Parser.hpp`)
 */
typedef int	(Parser::*ActionFunction)(Context&);

/**
 * @brief Structure that represents an action associated with a state
 *        transition. It stores the next parser state and the function
 *        to execute when the transition occurs.
 *
 * Composition:
 * 
 * - ParserState: the next state in the transition.
 * 
 * - ActionFunction: the function to execute during the transition.
 */
struct Action
{
    ParserState     nextState; /** next state in transition */
    ActionFunction  function; /** function to execute */
};

/**
 * @brief Pair of State & Event. The values contained in the corresponding
 * `enums`, servers as keys in order to find the corresponding Action
 * in the `std::map<TransitionKey, Action>`
 */
typedef std::pair<ParserState, ParserEvent>	TransitionKey;

class	StateMachine
{
	private:
		ParserState	initialState;
		ParserState	currentState;
		std::map<TransitionKey, Action>	functions;
		void	setCurrentState(ParserState state);
	public:
		StateMachine();
		StateMachine(ParserState initialState);
		StateMachine(const StateMachine& other);
		void	addTransition(ParserState fromState,
								ParserEvent event,
								ParserState toState,
								ActionFunction function);
		Action	nextTransition(ParserState currentState, ParserEvent event);
		void	handle(Context& ctx, Parser& parser, ParserEvent event);
		ParserState	getCurrentState();
};

#endif