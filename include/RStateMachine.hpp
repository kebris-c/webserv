/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RStateMachine.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 16:31:07 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 21:45:23 by kmarrero         ###   ########.fr       */
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
	REQ_ERROR,
	REQ_WAIT
};

enum	RequestEvent
{
	REQ_WAIT_INFO,
	REQ_GET_REQUEST,
	REQ_GET_HEADERS,
	REQ_GET_BODY,
	REQ_GET_VALUE,
	REQ_GET_INFO,
	REQUEST_END_EVENT
};

struct	RequestContext
{
	std::string		buffer;
	std::string		error;
	std::string		line;
};

typedef	RequestState	(RequestParser::*RequestFunction)(RequestContext&, Request&);

struct	RequestAction
{
	RequestFunction	function;
};

typedef	std::pair<RequestState, RequestEvent>	RequestTransitionKey;

class	RStateMachine
{
	private:
		RequestState	initialState;
		RequestState	currentState;
		std::map<RequestTransitionKey, RequestAction>	functions;
		void			setCurrentState(RequestState state);
	public:
		RStateMachine();
		RStateMachine(RequestState initialState);
		RStateMachine(const RStateMachine& other);
		~RStateMachine();
		RStateMachine&	operator=(const RStateMachine& other);
		void			addTransition(RequestState fromState,
								RequestEvent event,
								RequestFunction function);
		RequestAction	nextTransition(RequestState fromState, RequestEvent event);
		void			handle(RequestContext& ctx, RequestParser& parser, Request& request, RequestEvent event);
		RequestState	getCurrentState();
		RequestEvent	getNextEvent(RequestState fromState);
};

#endif