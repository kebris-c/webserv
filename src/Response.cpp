/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 19:29:03 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/28 19:30:21 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"
#include "Response.hpp"
#include "Router.hpp"

namespace {

bool	sameHeaderName(const std::string &left, const std::string &right)
{
	if (left.size() != right.size())
		return (false);
	for (std::size_t i = 0; i < left.size(); ++i)
	{
		char a = left[i];
		char b = right[i];
		if (a >= 'A' && a <= 'Z')
			a += 'a' - 'A';
		if (b >= 'A' && b <= 'Z')
			b += 'a' - 'A';
		if (a != b)
			return (false);
	}
	return (true);
}

}

Response::Response()
	: _status(200)
{
	_extensionDispatcher["html"] =	"text/html";
	_extensionDispatcher["htm"] =	"text/html";
	_extensionDispatcher["css"] =	"text/css";
	_extensionDispatcher["js"] =	"application/javascript";
	_extensionDispatcher["png"] =	"image/png";
	_extensionDispatcher["jpg"] =	"image/jpg";
	_extensionDispatcher["pdf"] =	"application/pdf";
	_extensionDispatcher["txt"] =	"text/txt";
}
Response::~Response() {}

void	Response::setStatus(int code) { _status = code; }

void	Response::setHeader(const std::string &key, const std::string &value)
{
	std::multimap<std::string, std::string>::iterator it = _headers.begin();

	while (it != _headers.end())
	{
		if (sameHeaderName(it->first, key))
			_headers.erase(it++);
		else
			++it;
	}
	addHeader(key, value);
}

void	Response::addHeader(const std::string &key, const std::string &value)
{
	_headers.insert(std::make_pair(key, value));
}

void	Response::setBody(const std::string &body)
{
	_body = body;
	std::ostringstream length;
	length << _body.size();
	setHeader("Content-Length", length.str());
}

std::string Response::getExtension(const std::string &path)
{
    std::string::size_type slash;
    std::string::size_type dot;

    slash = path.find_last_of('/');
    dot = path.find_last_of('.');

    if (dot == std::string::npos)
        return ("");

    if (slash != std::string::npos && dot < slash)
        return ("");

    return (path.substr(dot + 1));
}

bool Response::setBodyFromFile(const std::string &path)
{
    std::string extension;
    std::string contentType;
    std::map<std::string, std::string>::const_iterator it;
    std::ifstream file(path.c_str(), std::ios::binary);

    if (!file.is_open())
        return (false);
    std::istreambuf_iterator<char> begin(file);
    std::istreambuf_iterator<char> end;
    std::string body(begin, end);

    extension = getExtension(path);
    it = _extensionDispatcher.find(extension);
    if (it != _extensionDispatcher.end())
        contentType = it->second;
    else
        contentType = "application/octet-stream";

    setHeader("Content-Type", contentType);
    setBody(body);
    return (true);
}

std::string	Response::raw() const
{
	std::ostringstream	oss;
	bool noBody = (_status < 200 || _status == 204 || _status == 304);

	oss << "HTTP/1.1 " << _status << " " << utils::statusReason(_status) << "\r\n";
	for (std::multimap<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it)
	{
		if (!noBody || !sameHeaderName(it->first, "Content-Length"))
			oss << it->first << ": " << it->second << "\r\n";
	}
	oss << "\r\n";
	if (!noBody)
		oss << _body;
	return (oss.str());
}

Response	Response::makeError(int code, const ServerConfig *server)
{
	Response	r;

	r.setStatus(code);
	if (server != NULL)
	{
		std::map<int, std::string>::const_iterator page = server->errorPages.find(code);
		if (page != server->errorPages.end() && !page->second.empty())
		{
			Router		router;
			std::string	path = page->second;

			if (!path.empty() && path[0] == '/')
				path = router.resolve(*server, page->second);
			if (!path.empty() && r.setBodyFromFile(path))
				return (r);
		}
	}
	r.setHeader("Content-Type", "text/html");
	r.setBody("<html><body><h1>" + utils::toString(code) + " "
			  + utils::statusReason(code) + "</h1></body></html>");
	return (r);
}
