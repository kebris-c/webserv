/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:42:41 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/23 16:41:55 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestUtils.hpp"

bool	checkVersion(std::string& word)
{
	if (word != "HTTP/1.1")
		return (false);
	return (true);
}

bool	checkMethod(std::string& word)
{
	if (word != "GET" && word != "POST" && word != "DELETE")
		return (false);
	return (true);
}

bool	checkTarget(std::string& word)
{
	if (word[0] == '/')
		return (true);
	return (false);
}

bool	checkNameHeader(std::string& name, Context& ctx)
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

bool	checkValueHeader(std::string& value, Context& ctx)
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

RequestState	contentLenghtHeader(std::string& header, Context& ctx, Request& request)
{
	for (size_t i = 0; i < header.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(header[i])))
			return (request.setError("HEADER -> Content-Length: format not valid",
					ctx, REQ_ERROR, 400), REQ_BODY);
	}
	return (REQ_BODY);
}

RequestState	transferEncodingHeader(std::string& header, Context& ctx, Request& request)
{
	if (header == "chunked")
		return (REQ_BODY);
	return (request.setError("HEADER -> Transfer-Encoding: unsupported encoding",
			ctx, REQ_ERROR, 400), REQ_ERROR);
}

RequestState	connectionHeader(std::string& header, Context& ctx, Request& request)
{
	if (header == "keep-alive" || header == "close")
		return (REQ_BODY);
	return (request.setError("Header -> Connection-Header: invalid status",
			ctx, REQ_ERROR, 400), REQ_ERROR);
}
