/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 16:17:35 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/28 16:33:40 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestParser.hpp"
#include "RequestUtils.hpp"

RequestParser::RequestParser()
{
	headerHelpers["Content-Lenght"] = &contentLengthHeader;
	headerHelpers["Transfer-Encoding"] = &transferEncodingHeader;
	headerHelpers["Connection"] = &connectionHeader;
}

RequestParser::~RequestParser()
{}

std::vector<std::string>	RequestParser::requestSplit(const std::string& str, char delimiter)
{
	std::vector<std::string>	result;
	std::string::size_type		start = 0;
	std::string::size_type		pos;

	while ((pos = str.find(delimiter, start)) != std::string::npos)
	{
		result.push_back(str.substr(start, pos - start));
		start = pos + 1;
	}
	result.push_back(str.substr(start));
	return (result);
}

RequestState	RequestParser::checkFeed(RequestContext& ctx, Request& request)
{
	RequestState	state = request.getStateMachineState();
	RequestState	requestState = request.getCurrentState();

 	if (state == REQ_WAIT || state == REQ_FEED)
	{
		if (requestState == REQ_HEADERS)
		{
			if (ctx.buffer.find("\r\n\r\n") != std::string::npos)
				return (REQ_HEADERS);
			else if (ctx.buffer == "\r\n")
				return (REQ_ERROR);
			else
				return (REQ_WAIT);
		}
		else if (requestState == REQ_LINE)
		{
			if (ctx.buffer.find("\r\n") != std::string::npos)
				return (REQ_LINE);
			else if (ctx.buffer == "\r\n")
				return (REQ_ERROR);
			else
				return (REQ_WAIT);
		}
	}
	return (request.state());
}

RequestState	RequestParser::parseRequestLine(RequestContext& ctx, Request& request)
{
	std::string::size_type		pos;
	std::vector<std::string>	line;
	std::string					requestLine;

	pos = ctx.buffer.find("\r\n");
	if (pos == std::string::npos)
		return (REQ_WAIT);
	requestLine = ctx.buffer.substr(0, pos);
	line = requestSplit(requestLine, ' ');
	if (line.size() != 3)
		return (request.setError("Invalid header", ctx, REQ_ERROR, 400), REQ_ERROR);
	request.setMethod(line[0]);
	if (!checkMethod(request.method()))
		return (request.setError("No valid method", ctx, REQ_ERROR, 400), REQ_ERROR);
	request.setTarget(line[1]);
	if (!checkTarget(request.target()))
		return (request.setError("Invalid target format", ctx, REQ_ERROR, 400), REQ_ERROR);
	prepareTarget(request.target(), request);
	request.setVersion(line[2]);
	if (!checkVersion(request.version()))
		return (request.setError("version error", ctx, REQ_ERROR, 400), REQ_ERROR);
	request.setCurrentState(REQ_HEADERS);
	ctx.buffer.erase(0, pos + 2);
	return (request.state());
}

std::vector<std::string>	RequestParser::headerBodySplit(Request& request)
{
	std::vector<std::string>	vector;
	std::string::size_type		start = 0;
	std::string::size_type		pos;

	while ((pos = request.getContext().buffer.find("\r\n")) != std::string::npos)
	{
		vector.push_back(request.getContext().buffer.substr(start, pos - start));
		start = pos + 2;
	}
	if (start < request.getContext().buffer.size())
		vector.push_back(request.getContext().buffer.substr(start));
	return (vector);
}

RequestState	RequestParser::headersChecker(RequestContext& ctx, Request& request)
{
	RequestState											answer;
	std::map<std::string, std::string>::const_iterator		host;
	std::map<std::string, RequestHelpFunction>::iterator	loc;

	host = request.headers().find("Host");
	if (host == request.headers().end())
		return (request.setError("HEADER: Host not found",
				ctx, REQ_ERROR, 400), REQ_ERROR);
	for (std::map<std::string, std::string>::const_iterator it = request.headers().begin();
			it != request.headers().end(); ++it)
	{
		loc = headerHelpers.find(it->first);
		if (loc == headerHelpers.end())
			continue ;
		answer = loc->second(it->second, ctx, request);
		if (answer != REQ_ERROR)
			continue ;
		else
			break ;
	}
	if (!answer)
		return (REQ_COMPLETE);
	return (answer);
}

RequestState	RequestParser::parseRequestHeader(RequestContext& ctx, Request& request)
{
	std::string					name;
	std::string					value;
	std::string::size_type		pos;
	std::string::size_type		colon;
	std::vector<std::string>	line;
	RequestState				answer;

	pos = ctx.buffer.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (REQ_WAIT);
	line = headerBodySplit(request);
	for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); ++it)
	{
		colon = it->find(":");
		if (colon == std::string::npos)
			return (request.setError("HEADER: colon (:) not found", ctx, REQ_ERROR, 400), REQ_ERROR);
		if (colon == 0)
			return (request.setError("HEADER: colon (:) not found", ctx, REQ_ERROR, 400), REQ_ERROR);
		if ((*it)[colon - 1] == ' ')
			return (request.setError("HEADER: extra space not valid", ctx, REQ_ERROR, 400), REQ_ERROR);
		if ((*it)[colon - 1] == '\t')
			return (request.setError("HEADER: invalir character", ctx, REQ_ERROR, 400), REQ_ERROR);
		name = it->substr(0, colon);
		if (!checkNameHeader(name, ctx))
			return (request.setError("HEADER NAME: ", ctx, REQ_ERROR, 400), REQ_ERROR);
		value = it->substr(colon + 1);
		if (!checkValueHeader(value, ctx))
			return (request.setError("HEADER VALUE: ", ctx, REQ_ERROR, 400), REQ_ERROR);
		request.setHeaders(name, value, ctx);
	}
	answer = headersChecker(ctx, request);
	if (answer == REQ_ERROR)
		return (answer);
	ctx.buffer.erase(0, pos + 4);
	return (request.setCurrentState(answer), request.state());
}

std::string	RequestParser::parseChunkedBody(std::vector<std::string>& phrases, Request& request)
{
	std::string			buffer;
	std::string			hex = "0123456789abcdefABCDEF";
	int					hexValue;
	char				c;
	RequestContext		ctx = request.getContext();

	for (size_t i = 0; i < phrases.size(); ++i)
	{
		if (phrases[i] == "")
		{
			return (request.setError(" -> " + phrases[i]
				+ " must not be an empty string", ctx, REQ_ERROR, 400), "");
			break ;
		}
		c = phrases[i][0];
		if (hex.find(c) == std::string::npos)
		{
			return (request.setError(" -> " + std::string(1, c)
				+ " must be a valid hexadecimal value", ctx, REQ_ERROR, 400), "");
			break ;
		}
		std::stringstream	ss;
		ss << std::hex << c;
		ss >> hexValue;
		++i;
		buffer += phrases[i].substr(0, hexValue);
	}
	return (buffer);
}

RequestState	RequestParser::parseRequestBody(RequestContext& ctx, Request& request)
{
	std::vector<std::string>	chunkedInfo;
	std::string				body;
	std::string::size_type	pos;
	int						hexValue;

	body = obtainBodyInfo(request);
	if (body == "")
		return (request.setError("BODY: no body recieved", ctx, REQ_ERROR, 400), REQ_ERROR);
	if (request.chunked())
		chunkedInfo = headerBodySplit(request);
	if (chunkedInfo.size() > 0)
		parseChunkedBody(chunkedInfo, request);
	
	(void)ctx;
	(void)request;
	return (REQ_COMPLETE);
}
