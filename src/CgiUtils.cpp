/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiUtils.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 16:01:39 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/31 16:46:39 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "CgiUtils.hpp"
#include "HttpHandler.hpp"

std::string	lowerCgiName(std::string name)
{
	for (std::size_t i = 0; i < name.size(); ++i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
			name[i] += 'a' - 'A';
	}
	return (name);
}

std::string	trimCgiValue(const std::string &value)
{
	std::size_t first;
	std::size_t last;

	first = value.find_first_not_of(" \t");
	last = value.find_last_not_of(" \t");
	if (first == std::string::npos)
		return ("");
	return (value.substr(first, last - first + 1));
}

bool	validCgiName(const std::string &name)
{
	char				c;
	const std::string	punctuation = "!#$%&'*+-.^_`|~";

	if (name.empty())
		return (false);
	for (std::size_t i = 0; i < name.size(); ++i)
	{
		c = name[i];
		if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')
			&& punctuation.find(c) == std::string::npos)
			return (false);
	}
	return (true);
}

bool	parseCgiHeader(const std::string &line, CgiHeaders &headers)
{
	unsigned char	c;
	std::size_t colon;
	std::string	name;
	std::string	value;

	colon = line.find(":");
	if (colon == std::string::npos)
		return (false);
	name = lowerCgiName(line.substr(0, colon));
	value = trimCgiValue(line.substr(colon + 1));
	if (!validCgiName(name))
		return (false);
	for (std::size_t i = 0; i < value.size(); ++i)
	{
		c = static_cast<unsigned char>(value[i]);
		if ((c < 32 && c != '\t') || c == 127)
			return (false);
	}
	if (name == "status" || name == "content-type"
		|| name == "location" || name == "content-length")
	{
		if (value.empty() || headers.count(name) != 0)
			return (false);
	}
	headers.insert(std::make_pair(name, value));
	return (true);
}

/* Advance only through headers; everything after the blank line is opaque. */
bool	readCgiHeaders(const std::string &output, CgiHeaders &headers,
					std::size_t &bodyStart)
{
	bodyStart = 0;
	while (bodyStart < output.size())
	{
		std::size_t end = output.find('\n', bodyStart);
		if (end == std::string::npos)
			return (false);
		std::string line = output.substr(bodyStart, end - bodyStart);
		bodyStart = end + 1;
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			return (!headers.empty());
		if (!parseCgiHeader(line, headers))
			return (false);
	}
	return (false);
}

bool	parseCgiStatus(const std::string &value, int &status)
{
	if (value.size() < 3 || (value.size() > 3 && value[3] != ' '))
		return (false);
	status = 0;
	for (std::size_t i = 0; i < 3; ++i)
	{
		if (value[i] < '0' || value[i] > '9')
			return (false);
		status = status * 10 + value[i] - '0';
	}
	/* This API returns one final response, not informational responses. */
	return (status >= 200 && status <= 599);
}

bool	parseCgiLength(const std::string &value, std::size_t &length)
{
	length = 0;
	for (std::size_t i = 0; i < value.size(); ++i)
	{
		if (value[i] < '0' || value[i] > '9')
			return (false);
		std::size_t digit = value[i] - '0';
		if (length > (static_cast<std::size_t>(-1) - digit) / 10)
			return (false);
		length = length * 10 + digit;
	}
	return (!value.empty());
}

bool	cgiHopByHop(const std::string &name)
{
	return (name == "connection" || name == "keep-alive"
		|| name == "proxy-connection" || name == "te" || name == "trailer"
		|| name == "upgrade" || name == "transfer-encoding"
		|| name == "proxy-authenticate" || name == "proxy-authorization");
}

bool	cgiConnectionFields(const CgiHeaders &headers,
						std::map<std::string, bool> &excluded)
{
	std::pair<CgiHeaders::const_iterator, CgiHeaders::const_iterator> fields
		= headers.equal_range("connection");

	for (CgiHeaders::const_iterator it = fields.first; it != fields.second; ++it)
	{
		std::size_t start = 0;
		while (true)
		{
			std::size_t comma = it->second.find(',', start);
			std::string name = lowerCgiName(trimCgiValue(
				it->second.substr(start, comma == std::string::npos
					? std::string::npos : comma - start)));
			if (!validCgiName(name) || name == "status"
				|| name == "content-type" || name == "content-length"
				|| name == "location")
				return (false);
			excluded[name] = true;
			if (comma == std::string::npos)
				break ;
			start = comma + 1;
		}
	}
	return (true);
}