/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RStateMachine.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 16:31:07 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/22 19:32:09 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_STATE_MACHINE
# define REQUEST_STATE_MACHINE

# include "Webserv.hpp"

enum RequestState
{
	REQ_FEED,
	REQ_LINE,
	REQ_HEADERS,
	REQ_BODY,
	REQ_COMPLETE,
	REQ_ERROR
};

enum	RequestEvent
{
	REQ_READ,
	REQ_GET_REQUEST,
	REQ_GET_HEADERS,
	END_EVENT
};

struct	Context
{
	RequestState	state;
	std::string		buffer;
	std::string		error;
};

typedef	RequestState	(Request::*ActionFunction)(Context&);

struct	Action
{
	ActionFunction	function;
};

typedef	std::pair<RequestState, RequestEvent>	TransitionKey;

class	RStateMachine
{
	private:
		RequestState	initialState;
		RequestState	currentState;
		std::map<TransitionKey, Action>	functions;
		void			setCurrentState(RequestState state);
	public:
		RStateMachine();
		RStateMachine(RequestState initialState);
		RStateMachine(const RStateMachine& other);
		~RStateMachine();
		RStateMachine&	operator=(const RStateMachine& other);
		void			addTransition(RequestState fromState,
								RequestEvent event,
								ActionFunction function);
		Action			nextTransition(RequestState fromState, RequestEvent event);
		void			handle(Context& ctx, Request& request, RequestEvent event);
		RequestState	getCurrentState();
		RequestEvent	getNextEvent(RequestState fromState);
};

#endif