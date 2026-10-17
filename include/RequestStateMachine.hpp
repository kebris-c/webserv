/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestStateMachine.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 16:31:07 by kmarrero          #+#    #+#             */
/*   Updated: 2026/10/17 16:04:42 by kjroydev         ###   ########.fr       */
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
	REQ_GET_CHUNKED_BODY,
	REQ_GET_VALUE,
	REQ_GET_INFO,
	REQUEST_END_EVENT
};

enum	ChunkState
{
	CHUNK_START,
	CHUNK_SIZE,
	CHUNK_DATA,
	CHUNK_DATA_CRLF,
	CHUNK_TRAILERS,
	CHUNK_COMPLETE
};

struct	RequestContext
{
	std::string		buffer;
	std::string		error;
	std::string		line;
	size_t			chunkSize; /** Bytes the current chunk needs. */
	size_t			chunkDigitEnd;
	size_t			chunkEnd;
	size_t			chunkOffSet;
	ChunkState		chunkState; /** Current state of the chunk. */
	std::string		chunkedBody; /** Accumulated body of the chunk. */
	bool			chunked;
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
		std::map<RequestTransitionKey, RequestAction>	functions;
		RequestState									initialState;
		RequestState									currentState;
		void											setCurrentState(RequestState state);
	public:
		RStateMachine();
		RStateMachine(RequestState initialState);
		RStateMachine(const RStateMachine& other);
		~RStateMachine();
		RStateMachine&	operator=(const RStateMachine& other);
		RequestState	getCurrentState();
		RequestEvent	getNextEvent(RequestState fromState, RequestContext& ctx);
		RequestAction	nextTransition(RequestState fromState, RequestEvent event);
		void			addTransition(RequestState fromState,
								RequestEvent event,
								RequestFunction function);
		void			handle(RequestContext& ctx,
								RequestParser& parser,
								Request& request,
								RequestEvent event);
};

#endif