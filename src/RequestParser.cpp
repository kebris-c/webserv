/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 16:17:35 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 18:27:39 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestParser.hpp"
#include "RequestUtils.hpp"

RequestParser::RequestParser()
{
	headerHelpers["Content-Lenght"] = &contentLenghtHeader;
	headerHelpers["Transfer-Encoding"] = &transferEncodingHeader;
	headerHelpers["Connection"] = &connectionHeader;
}

RequestParser::~RequestParser()
{}

std::vector<std::string>	RequestParser::split(const std::string& str, char delimiter)
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

RequestState	RequestParser::parseRequestLine(RequestContext& ctx, Request& request)
{
	std::string::size_type		pos;
	std::vector<std::string>	line;
	std::string					requestLine;

	pos = ctx.buffer.find("\r\n");
	requestLine = ctx.buffer.substr(0, pos);
	line = split(requestLine, ' ');
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

std::vector<std::string>	RequestParser::headerSplit(const std::string& str)
{
	std::vector<std::string>	result;
	std::string::size_type		start = 0;
	std::string::size_type		pos;

	while ((pos = str.find("\r\n", start)) != std::string::npos)
	{
		result.push_back(str.substr(start, pos - start));
		start = pos + 2;
	}
	if (start < str.size())
		result.push_back(str.substr(start));
	return (result);
}

RequestState	RequestParser::headersChecker(RequestContext& ctx, Request& request)
{
	RequestState									answer;
	std::map<std::string, Function>::iterator		loc;
	std::map<std::string, std::string>::const_iterator	host;

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
	return (answer);
}

RequestState	RequestParser::parseRequestHeader(RequestContext& ctx, Request& request)
{
	std::string					name;
	std::string					value;
	std::string					headerLines;
	std::string::size_type		pos;
	std::string::size_type		colon;
	std::vector<std::string>	line;
	RequestState				answer;

	pos = ctx.buffer.find("\r\n\r\n");
	headerLines = ctx.buffer.substr(0, pos);
	line = headerSplit(headerLines);
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
	return (answer);
}
