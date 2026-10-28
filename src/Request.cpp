/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 19:28:45 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/28 19:28:48 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include "RequestParser.hpp"

Request::Request()
    : _errorCode(0),
      _chunked(false),
      _contentLength(0),
      _state(REQ_LINE)
{
	configureStateMachine();
	_ctx.chunked = false;
	_ctx.chunkedBody = "";
	_ctx.chunkSize = 0;
	_ctx.chunkDigitEnd = 0;
	_ctx.chunkEnd = 0;
	_ctx.chunkOffSet = 0;
}

Request::~Request()
{}

void	Request::configureStateMachine()
{
	_stateMachine = RStateMachine();
	_stateMachine.addTransition(REQ_FEED, REQ_WAIT_INFO, &RequestParser::checkFeed);
	_stateMachine.addTransition(REQ_FEED, REQ_GET_REQUEST, &RequestParser::checkFeed);
	_stateMachine.addTransition(REQ_WAIT, REQ_WAIT_INFO, &RequestParser::checkFeed);
	_stateMachine.addTransition(REQ_LINE, REQ_GET_REQUEST, &RequestParser::parseRequestLine);
	_stateMachine.addTransition(REQ_HEADERS, REQ_GET_HEADERS, &RequestParser::parseRequestHeader);
	_stateMachine.addTransition(REQ_BODY, REQ_GET_BODY, &RequestParser::parseRequestBody);
	_stateMachine.addTransition(REQ_BODY, REQ_GET_CHUNKED_BODY, &RequestParser::parseChunkedBody);
}

void	Request::reset()
{
	configureStateMachine();
	_ctx.buffer.clear();
	_ctx.line.clear();
	_ctx.error.clear();
	_ctx.chunked = false;
	_ctx.chunkedBody = "";
	_ctx.chunkSize = 0;
	_ctx.chunkDigitEnd = 0;
	_ctx.chunkEnd = 0;
	_ctx.chunkOffSet = 0;
	_state = REQ_LINE;
	_method.clear();
	_target.clear();
	_query.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_contentLength = 0;
	_chunked = false;
	_errorCode = 0;
}

void	Request::setServer(std::vector<ServerConfig>& server)
{
	this->_server = server;
}

void	Request::setCurrentState(RequestState state)
{
	this->_state = state;
}

void	Request::setError(std::string message, RequestContext& ctx, RequestState state, int errorCode)
{
	_state = state;
	_errorCode = errorCode;
	if (ctx.error.empty())
		ctx.error = message;
	else
		ctx.error = message + ctx.error;
}

void	Request::setMethod(std::string& method)
{
	this->_method = method;
}

void	Request::setTarget(std::string& target)
{
	this->_target = target;
}

void	Request::setQuery(std::string& query)
{
	this->_query = query;
}

void	Request::setVersion(std::string& version)
{
	this->_version = version;
}

void	Request::setBody(std::string& body)
{
	this->_body = body;
}

void	Request::setContentLength(std::size_t& length)
{
	this->_contentLength = length;
}

void	Request::setHeaders(std::string name, std::string value, RequestContext& ctx)
{
	if (name.empty())
		return (setError("HEADER: header name it's empty", ctx, REQ_ERROR, 400), (void)NULL);
	if (_headers.find(name) == _headers.end())
	{
		_headers[name] = value;
		if (name == "Transfer-Encoding")
		{
			if (value == "chunked")
			{
				_chunked = true;
				ctx.chunked = true;
			}
		}
		else
			return ;
	}
	else
	{
		setError("HEADER " + name + ": already exist in map", ctx, REQ_ERROR, 400);
		return ;
	}
}

void	Request::setErrorCode(int& code)
{
	this->_errorCode = code;
}

bool	Request::feed(const std::string& data, RequestParser& parser)
{
	RequestState	state = _stateMachine.getCurrentState();

	_ctx.buffer += data;
	if (!_ctx.buffer.empty())
		state = getCurrentState();
	while (state != REQ_COMPLETE && state != REQ_ERROR && state != REQ_WAIT)
	{
		RequestEvent	event = _stateMachine.getNextEvent(state, _ctx);
	
		_stateMachine.handle(_ctx, parser, *this, event);
		state = _stateMachine.getCurrentState();
	}
	if (state != REQ_WAIT)
		setCurrentState(state);
	return (state != REQ_ERROR);
}

bool	Request::isComplete()
{
	return (this->_state == REQ_COMPLETE);
}

void	Request::print(RequestContext& ctx)
{
	std::cout << ctx.buffer << std::endl;
}

RequestState	Request::getCurrentState()
{
	return (this->_state);
}

RequestState	Request::getStateMachineState()
{
	return (this->_stateMachine.getCurrentState());
}

RequestContext	Request::getContext()
{
	return (this->_ctx);
}

bool	Request::parse(std::string &buffer, RequestParser& parser)
{
	if (!feed(buffer, parser))
		return (false);
	if (!isComplete())
		return (state() != REQ_ERROR);
	return (isComplete());
}

RequestState		Request::state() const { return (_state); }
const std::string	&Request::method() const { return (_method); }
const std::string	&Request::target() const { return (_target); }
const std::string	&Request::query() const { return (_query); }
const std::string	&Request::version() const { return (_version); }
const bool			&Request::chunked() const { return (_chunked); }
const std::map<std::string, std::string>	&Request::headers() const { return (_headers); }
const std::string	&Request::body() const { return (_body); }
const size_t		&Request::contentLength() const { return (_contentLength); }
int	Request::errorCode() const { return (_errorCode); }

std::string	Request::header(const std::string &name) const
{
	/* TODO(kmarrero): case-insensitive lookup */
	std::map<std::string, std::string>::const_iterator it = _headers.find(name);
	if (it == _headers.end())
		return ("");
	return (it->second);
}
