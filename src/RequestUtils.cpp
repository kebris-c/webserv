/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:42:41 by kmarrero          #+#    #+#             */
/*   Updated: 2026/10/17 17:20:03 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestUtils.hpp"

bool	checkTarget(const std::string& word)
{
	if (word.empty() || word[0] == '/')
		return (true);
	return (false);
}

bool	checkMethod(const std::string& word)
{
	if (word != "GET" && word != "POST" && word != "DELETE")
		return (false);
	return (true);
}

bool	checkVersion(const std::string& word)
{
	if (word != "HTTP/1.1")
		return (false);
	return (true);
}

bool	colonChecker(const std::string::size_type& colon,
						std::vector<std::string>::iterator it,
						Request& request,
						RequestContext& ctx)
{
	if (colon == std::string::npos)
		return (request.setError("HEADER: colon (:) not found",
					ctx, REQ_ERROR, 400), false);
	if (colon == 0)
		return (request.setError("HEADER: colon (:) not found",
					ctx, REQ_ERROR, 400), false);
	if ((*it)[colon - 1] == ' ')
		return (request.setError("HEADER: extra space not valid",
					ctx, REQ_ERROR, 400), false);
	if ((*it)[colon - 1] == '\t')
		return (request.setError("HEADER: invalir character",
					ctx, REQ_ERROR, 400), false);
	return (true);
}

bool	checkNameHeader(std::string& name, RequestContext& ctx)
{
	if (name.empty())
	{
		ctx.error += "Header name is empty";
		return (false);
	}
	for (std::string::const_iterator it = name.begin(); it != name.end(); ++it)
	{
		if (!std::isalnum(static_cast<unsigned char>(*it)) && *it != '-')
		{
			ctx.error += "Header contain no valid characters";
			return (false);
		}
	}
	return (true);
}

bool	checkValueHeader(std::string& value, RequestContext& ctx)
{
	unsigned char	c;

	while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
		value.erase(0, 1);
	while (!value.empty() && (value[value.size() - 1] == ' ' || value[value.size() - 1] == '\t'))
		value.erase(value.size() - 1);
	for (std::string::const_iterator it = value.begin(); it != value.end(); ++it)
	{
		c = static_cast<unsigned char>(*it);
		if (c < 32 || c == 127)
		{
			ctx.error += "Header value contains invalid characters";
			return (false);
		}
	}
	return (true);
}

void	prepareTarget(const std::string& target, Request& request)
{
	std::string::size_type	pos;
	std::string	objective;

	pos = target.find("?");
	if (pos != std::string::npos)
	{
		objective = target.substr(pos + 1);
		request.setQuery(objective);
	}
	else
		return ;
}

RequestState	getHexadecimalLength(RequestContext& ctx, Request& request)
{
	const std::string hex = "0123456789abcdef";

	for (size_t i = ctx.chunkOffSet; i < ctx.chunkDigitEnd; ++i)
	{
		size_t	digit = hex.find(std::tolower(static_cast<unsigned char>(ctx.buffer[i])));
		if (digit == std::string::npos || ctx.chunkSize > (static_cast<size_t>(-1) - digit) / 16)
			return (request.setError("Invalid chunk size", ctx, REQ_ERROR, 400), REQ_ERROR);
		ctx.chunkSize = ctx.chunkSize * 16 + digit;
	}
	return (REQ_COMPLETE);
}

RequestState	chunkCalculations(RequestContext& ctx, Request& request)
{
	ctx.chunkEnd = ctx.buffer.find("\r\n", ctx.chunkOffSet);
	if (ctx.chunkEnd == std::string::npos)
		return (REQ_WAIT);
	ctx.chunkDigitEnd = ctx.buffer.find(';', ctx.chunkOffSet);
	if (ctx.chunkDigitEnd == std::string::npos || ctx.chunkDigitEnd > ctx.chunkEnd)
		ctx.chunkDigitEnd = ctx.chunkEnd;
	if (ctx.chunkDigitEnd == ctx.chunkOffSet)
		return (request.setError("Empty chunk size", ctx, REQ_ERROR, 400), REQ_ERROR);
	if (getHexadecimalLength(ctx, request) == REQ_ERROR)
		return (REQ_ERROR);
	ctx.chunkOffSet = ctx.chunkEnd + 2;
	return (REQ_COMPLETE);
}

RequestState	consumeTrailerInFinalLine(RequestContext& ctx, Request& request)
{
	while (true)
	{
		ctx.chunkEnd = ctx.buffer.find("\r\n", ctx.chunkOffSet);
		if (ctx.chunkEnd == std::string::npos)
			return (REQ_WAIT);
		if (ctx.chunkEnd == ctx.chunkOffSet)
		{
			ctx.buffer.erase(0, ctx.chunkEnd + 2);
			request.setBody(ctx.chunkedBody);
			return (REQ_COMPLETE);
		}
		if (ctx.buffer.find(':', ctx.chunkOffSet) >= ctx.chunkEnd)
			return (request.setError("Invalid trailer", ctx, REQ_ERROR, 400), REQ_ERROR);
		ctx.chunkOffSet = ctx.chunkEnd + 2;
	}
}

RequestState	connectionHeader(const std::string& header, RequestContext& ctx, Request& request)
{
	if (header == "keep-alive" || header == "close")
		return (REQ_BODY);
	return (request.setError("Header -> Connection-Header: invalid status",
			ctx, REQ_ERROR, 400), REQ_ERROR);
}

RequestState	contentLengthHeader(const std::string& header, RequestContext& ctx, Request& request)
{
	unsigned long	value;
	char			*end;
	errno = 0;

	if (header.empty())
		return (request.setError("Empty Content-Length", ctx, REQ_ERROR, 400), REQ_ERROR);
	for (size_t i = 0; i < header.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(header[i])))
			return (request.setError("HEADER -> Content-Length: format not valid",
					ctx, REQ_ERROR, 400), REQ_ERROR);
	}
	value = std::strtoul(header.c_str(), &end, 10);
	if (errno == ERANGE || *end != '\0')
		return (request.setError("Content-Length VALUE -> there was an error in value",
			ctx, REQ_ERROR, 400), REQ_ERROR);
	request.setContentLength(value);
	return (REQ_BODY);
}

RequestState	transferEncodingHeader(const std::string& header, RequestContext& ctx, Request& request)
{
	if (header == "chunked")
		return (REQ_BODY);
	return (request.setError("HEADER -> Transfer-Encoding: unsupported encoding",
			ctx, REQ_ERROR, 400), REQ_ERROR);
}

std::string::size_type	obtainStatusFromContext(Request& request)
{
	std::string::size_type	pos;
	RequestContext			ctx = request.getContext();
	RequestState			state = request.state();

	if (state == REQ_LINE)
	{
		pos = ctx.buffer.find("\r\n");
		if (pos != std::string::npos)
			request.setCurrentState(REQ_LINE);
		else
			return (std::string::npos);
	}
	else if (state == REQ_HEADERS)
	{
		pos = ctx.buffer.find("\r\n\r\n");
		if (pos != std::string::npos)
			request.setCurrentState(REQ_HEADERS);
		else
			return (std::string::npos);
	}
	return (pos);
}

std::string	normalizeHeader(const std::string& header)
{
	std::string	name;
	const char *canonical[] = {"Host", "Content-Length",
								"Transfer-Encoding",
								"Connection", "Content-Type"};

	for (size_t i = 0; i < header.size(); ++i)
		name += std::tolower(static_cast<unsigned char>(header[i]));
	for (size_t j = 0; j < sizeof(canonical) / sizeof(canonical[0]); ++j)
	{
		std::string	lower = canonical[j];
		for (size_t i = 0; i < lower.size(); ++i)
			lower[i] = std::tolower(static_cast<unsigned char>(lower[i]));
		if (name == lower)
		{
			name = canonical[j];
			return (name);
		}
	}
	return (name);
}
