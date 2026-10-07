/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 16:17:35 by kmarrero          #+#    #+#             */
/*   Updated: 2026/10/07 16:57:16 by kjroydev         ###   ########.fr       */
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

 	if ((state == REQ_WAIT || state == REQ_FEED) && (!ctx.buffer.empty()))
	{
		if (requestState == REQ_LINE)
		{
			if (ctx.buffer == "\r\n")
				return (REQ_ERROR);
			else if (ctx.buffer.find("\r\n") != std::string::npos)
				return (REQ_LINE);
			else
				return (REQ_WAIT);
		}
		else if (requestState == REQ_HEADERS)
		{
			if (ctx.buffer == "\r\n")
				return (REQ_ERROR);
			else if (ctx.buffer.find("\r\n\r\n") != std::string::npos)
				return (REQ_HEADERS);
			else
				return (REQ_WAIT);
		}
		else if (requestState == REQ_BODY)
		{
			if (request.chunked())
			{
				if (ctx.buffer == "\r\n")
					return (REQ_ERROR);
				else if (ctx.buffer.find("0\r\n\r\n") != std::string::npos)
					return (REQ_BODY);
				else
					return (REQ_WAIT);
			}
			else
			{
				if (ctx.buffer == "\r\n")
					return (REQ_ERROR);
				else if (ctx.buffer.find("\r\n") != std::string::npos)
					return (REQ_BODY);
				else
					return (REQ_WAIT);
			}
		}
	}
	return (request.state());
}

RequestState	RequestParser::parseRequestLine(RequestContext& ctx, Request& request)
{
	std::string::size_type		pos = obtainStatusFromContext(request);
	std::vector<std::string>	line;
	std::string					requestLine;

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

std::vector<std::string>	RequestParser::headerSplit(Request& request)
{
	std::vector<std::string>	vector;
	std::string::size_type		pos;
	std::string::size_type		start = 0;

	while ((pos = request.getContext().buffer.find("\r\n", start)) != std::string::npos)
	{
		if (pos == start)
			break ;
		vector.push_back(request.getContext().buffer.substr(start, pos - start));
		start = pos + 2;
	}
	return (vector);
}

RequestState	RequestParser::headersChecker(RequestContext& ctx, Request& request)
{
	std::map<std::string, RequestHelpFunction>::iterator	loc;
	std::map<std::string, std::string>::const_iterator		host;
	RequestState											answer;

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
	std::string::size_type		pos = obtainStatusFromContext(request);
	std::string::size_type		colon;
	std::vector<std::string>	line;

	if (pos == std::string::npos)
		return (REQ_WAIT);
	line = headerSplit(request);
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
	RequestState answer = headersChecker(ctx, request);
	if (answer == REQ_ERROR)
		return (answer);
	ctx.buffer.erase(0, pos + 4);
	return (request.setCurrentState(answer), request.state());
}

std::vector<std::string>	RequestParser::bodySplit(std::string& body)
{
	std::vector<std::string>	bodyChunk;
	std::string::size_type		start = 0;
	std::string::size_type		pos;

	while ((pos = body.find("\r\n", start)) != std::string::npos)
	{
		bodyChunk.push_back(body.substr(start, (pos + 2) - start));
		start = pos + 2;
	}
	if (start < body.size())
		bodyChunk.push_back(body.substr(start));
	return (bodyChunk);
}

std::string	RequestParser::parseChunkedBody(std::vector<std::string>& phrases, Request& request, RequestContext& ctx)
{
	std::string			buffer;
	size_t				hexValue;

	for (size_t i = 0; i < phrases.size();)
	{
		if (phrases[i] == "")
		{
			return (request.setError(" -> " + phrases[i]
				+ " must not be an empty string", ctx, REQ_ERROR, 400), "");
			break ;
		}
		hexValue = obtainHexValue(phrases[i]);
		++i;
		i = bufferConstruct(hexValue, buffer, phrases, i);
	}
	return (buffer);
}

RequestState	RequestParser::parseRequestBody(RequestContext& ctx, Request& request)
{
	std::string::size_type		pos = obtainBodyInfo(request);
	std::string					body;
	std::vector<std::string>	chunkedInfo;

	if (pos == std::string::npos)
		return (REQ_WAIT);
	body = ctx.buffer.substr(0, pos);
	if (request.state() != REQ_BODY || body == "" || body == "\r\n")
		return (request.setError("BODY: no body recieved", ctx, REQ_ERROR, 400), REQ_ERROR);
	if (request.chunked())
	{
		chunkedInfo = bodySplit(body);
		if (chunkedInfo.size() > 0)
		{
			body = parseChunkedBody(chunkedInfo, request, ctx);
			request.setBody(body);
			return (REQ_COMPLETE);
		}
		else
			return (request.setError("BODY: no info recieved", ctx, REQ_ERROR, 400), REQ_ERROR);
	}
	size_t	length = request.contentLength();
	if (body.size() == length)
		request.setBody(body);
	else
		return (request.setError("BODY: the amount of bytes it's higher than announced",
				ctx, REQ_ERROR, 413), REQ_ERROR);
	return (REQ_COMPLETE);
}
