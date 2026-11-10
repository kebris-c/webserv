/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 19:29:27 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/28 19:29:29 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"

#include <cctype>

namespace utils {

std::string	trim(const std::string &s)
{
	std::size_t	start;
	std::size_t	end;

	start = 0;
	while (start < s.size()
		&& std::isspace(static_cast<unsigned char>(s[start])))
		++start;
	end = s.size();
	while (end > start
		&& std::isspace(static_cast<unsigned char>(s[end - 1])))
		--end;
	return (s.substr(start, end - start));
}

std::vector<std::string>	split(const std::string &s, char delim)
{
	std::vector<std::string>	out;
	std::string					cur;

	for (std::size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == delim)
		{
			out.push_back(cur);
			cur.clear();
		}
		else
			cur += s[i];
	}
	out.push_back(cur);
	return (out);
}

std::string	toString(int n)
{
	std::ostringstream	oss;
	oss << n;
	return (oss.str());
}

std::string	statusReason(int code)
{
	switch (code)
	{
	case 200: return "OK";
	case 201: return "Created";
	case 202: return "Accepted";
	case 203: return "Non-Authoritative Information";
	case 204: return "No Content";
	case 205: return "Reset Content";
	case 206: return "Partial Content";
	case 300: return "Multiple Choices";
	case 301: return "Moved Permanently";
	case 302: return "Found";
	case 303: return "See Other";
	case 304: return "Not Modified";
	case 307: return "Temporary Redirect";
	case 308: return "Permanent Redirect";
	case 400: return "Bad Request";
	case 401: return "Unauthorized";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	case 405: return "Method Not Allowed";
	case 406: return "Not Acceptable";
	case 408: return "Request Timeout";
	case 409: return "Conflict";
	case 410: return "Gone";
	case 411: return "Length Required";
	case 412: return "Precondition Failed";
	case 413: return "Content Too Large";
	case 414: return "URI Too Long";
	case 415: return "Unsupported Media Type";
	case 416: return "Range Not Satisfiable";
	case 417: return "Expectation Failed";
	case 422: return "Unprocessable Content";
	case 429: return "Too Many Requests";
	case 500: return "Internal Server Error";
	case 501: return "Not Implemented";
	case 502: return "Bad Gateway";
	case 503: return "Service Unavailable";
	case 504: return "Gateway Timeout";
	case 505: return "HTTP Version Not Supported";
	default:  return "Unknown";
	}
}

bool	fileExists(const std::string &path)
{
	struct stat	info;

	return (stat(path.c_str(), &info) == 0);
}

bool	isDirectory(const std::string &path)
{
	struct stat	info;

	return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode));
}

std::string	joinPath(const std::string &a, const std::string &b)
{
	std::string	right;
	std::size_t	start;

	if (b == "." || b == ".." || b.find("/../") != std::string::npos
		|| b.find("/./") != std::string::npos
		|| b.compare(0, 3, "../") == 0 || b.compare(0, 2, "./") == 0)
		return ("");
	if (a.empty())
		return (b);
	if (b.empty())
		return (a);
	right = b;
	start = 0;
	while (start < right.size() && right[start] == '/')
		++start;
	right = right.substr(start);
	if (!a.empty() && a[a.size() - 1] == '/')
		return (a + right);
	return (a + "/" + right);
}

} /* namespace utils */
