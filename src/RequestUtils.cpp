/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:42:41 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 21:17:31 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestUtils.hpp"

bool	checkVersion(const std::string& word)
{
	if (word != "HTTP/1.1")
		return (false);
	return (true);
}

bool	checkMethod(const std::string& word)
{
	if (word != "GET" && word != "POST" && word != "DELETE")
		return (false);
	return (true);
}

bool	checkTarget(const std::string& word)
{
	if (word[0] == '/')
		return (true);
	return (false);
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

RequestState	contentLengthHeader(const std::string& header, RequestContext& ctx, Request& request)
{
	unsigned long	value;
	char			*end;
	errno = 0;

	for (size_t i = 0; i < header.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(header[i])))
			return (request.setError("HEADER -> Content-Length: format not valid",
					ctx, REQ_ERROR, 400), REQ_BODY);
	}
	value = std::strtoul(header.c_str(), &end, 10);
	if (errno == ERANGE || *end != '\0')
		return (request.setError("Content-Length VALUE -> there was an error in value",
			ctx, REQ_ERROR, 400), REQ_ERROR);
	return (REQ_BODY);
}

RequestState	transferEncodingHeader(const std::string& header, RequestContext& ctx, Request& request)
{
	if (header == "chunked")
		return (REQ_BODY);
	return (request.setError("HEADER -> Transfer-Encoding: unsupported encoding",
			ctx, REQ_ERROR, 400), REQ_ERROR);
}

RequestState	connectionHeader(const std::string& header, RequestContext& ctx, Request& request)
{
	if (header == "keep-alive" || header == "close")
		return (REQ_BODY);
	return (request.setError("Header -> Connection-Header: invalid status",
			ctx, REQ_ERROR, 400), REQ_ERROR);
}
