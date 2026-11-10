/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 16:45:17 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/31 16:26:56 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiUtils.hpp"
#include "CgiProcess.hpp"
#include "HttpHandler.hpp"

namespace {

std::size_t	bodyLimit(const RouteMatch &route)
{
	if (route.location != NULL && route.location->clientMaxBodySize > 0)
		return (route.location->clientMaxBodySize);
	if (route.server != NULL)
		return (route.server->clientMaxBodySize);
	return (0);
}

std::string	uploadFile(const Request &req, const LocationConfig &loc)
{
	std::string	rel;
	std::string	path;

	if (loc.uploadStore.empty())
		return ("");
	rel = req.target();
	if (rel.size() >= loc.path.size()
		&& rel.compare(0, loc.path.size(), loc.path) == 0)
		rel = rel.substr(loc.path.size());
	while (!rel.empty() && rel[0] == '/')
		rel.erase(0, 1);
	if (rel.empty() || rel == "." || rel == ".."
		|| rel.find('/') != std::string::npos)
		return ("");
	path = loc.uploadStore;
	if (!path.empty() && path[path.size() - 1] != '/')
		path += "/";
	path += rel;
	return (path);
}

}

HttpHandler::HttpHandler()
{}

HttpHandler::~HttpHandler()
{}

Response	HttpHandler::handle(const Request &req, const RouteMatch &route) const
{
	if (route.location == NULL)
		return (Response::makeError(404, route.server));

	const LocationConfig	&loc = *route.location;
	std::string				method;

	if (!loc.redirect.empty())
		return (handleRedirect(loc));
	if (!route.ok || route.fsPath.empty())
		return (Response::makeError(404, route.server));
	if (!methodAllowed(loc, req.method()))
		return (Response::makeError(405, route.server));
	method = req.method();
	if (method == "GET")
		return (handleGet(route));
	if (method == "POST")
		return (handlePost(req, route));
	if (method == "DELETE")
		return (handleDelete(route));
	return (Response::makeError(501, route.server));
}

/*
 * Exposes the asynchronous CGI decision to Server without performing I/O.
 * remoteAddr is Connection::remoteAddr() (dotted IPv4) for REMOTE_ADDR.
 */
bool	HttpHandler::prepareCgi(const Request &req, const RouteMatch &route,
								const std::string &remoteAddr,
								std::string &scriptPath,
								std::vector<std::string> &environment) const
{
	std::size_t	limit;

	if (route.server == NULL || route.location == NULL || !route.ok
		|| req.state() != REQ_COMPLETE || route.fsPath.empty()
		|| !route.location->redirect.empty())
		return (false);

	const LocationConfig	&loc = *route.location;

	environment.clear();
	scriptPath.clear();
	limit = bodyLimit(route);
	if (loc.cgiPass.empty() || loc.cgiExtension.empty()
		|| !methodAllowed(loc, req.method())
		|| (req.method() != "GET" && req.method() != "POST")
		|| (limit > 0 && req.body().size() > limit)
		|| route.fsPath.size() < loc.cgiExtension.size()
		|| route.fsPath.compare(route.fsPath.size() - loc.cgiExtension.size(),
				loc.cgiExtension.size(), loc.cgiExtension) != 0)
		return (false);
	environment = CgiProcess::buildEnv(req, *route.server, loc, route.fsPath);
	environment.push_back("REMOTE_ADDR=" + remoteAddr);
	scriptPath = route.fsPath;
	return (true);
}

/*
 * Converts complete CGI stdout into an HTTP response after pipe EOF.
 */
Response	HttpHandler::parseCgiOutput(const std::string &output,
								const ServerConfig *server) const
{
	Response					response;
	CgiHeaders					headers;
	std::map<std::string, bool>	excluded;
	std::size_t					bodyStart;
	std::size_t					length;
	int							status = 200;

	if (!readCgiHeaders(output, headers, bodyStart)
		|| !cgiConnectionFields(headers, excluded))
		return (Response::makeError(500, server));
	CgiHeaders::const_iterator field = headers.find("status");
	if (field != headers.end())
	{
		if (!parseCgiStatus(field->second, status))
			return (Response::makeError(500, server));
	}
	else if (headers.count("location") != 0)
		status = 302;
	std::size_t bodySize = output.size() - bodyStart;
	bool noBody = (status == 204 || status == 205 || status == 304);
	if ((noBody && bodySize != 0)
		|| (!noBody && headers.count("content-type") == 0
			&& !(status >= 300 && status < 400
				&& headers.count("location") != 0 && bodySize == 0)))
		return (Response::makeError(500, server));
	field = headers.find("content-length");
	if (field != headers.end()
		&& (!parseCgiLength(field->second, length)
			|| (status != 304 && length != bodySize)))
		return (Response::makeError(500, server));
	/* CGI supplies body bytes, never HTTP chunk framing. */
	if (headers.count("transfer-encoding") != 0)
		return (Response::makeError(500, server));
	response.setStatus(status);
	for (field = headers.begin(); field != headers.end(); ++field)
	{
		if (field->first != "status" && field->first != "content-length"
			&& !cgiHopByHop(field->first) && excluded.count(field->first) == 0
			&& field->first.compare(0, 6, "x-cgi-") != 0)
			response.addHeader(field->first, field->second);
	}
	response.setBody(output.substr(bodyStart));
	return (response);
}

Response	HttpHandler::handleGet(const RouteMatch &route) const
{
	struct stat		fileInfo;
	Response		response;
	std::string		indexPath;

	if (stat(route.fsPath.c_str(), &fileInfo) == -1)
	{
		if (errno == EACCES)
			return (Response::makeError(403, route.server));
		if (errno == ENOENT || errno == ENOTDIR)
			return (Response::makeError(404, route.server));
		return (Response::makeError(500, route.server));
	}
	if (S_ISREG(fileInfo.st_mode))
	{
		if (!response.setBodyFromFile(route.fsPath))
			return (Response::makeError(500, route.server));
		return (response);
	}
	if (S_ISDIR(fileInfo.st_mode))
	{
		if (!route.location->index.empty())
		{
			indexPath = route.fsPath;
			if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
				indexPath += "/";
			indexPath += route.location->index;
			if (stat(indexPath.c_str(), &fileInfo) == 0 && S_ISREG(fileInfo.st_mode))
			{
				if (!response.setBodyFromFile(indexPath))
					return (Response::makeError(500, route.server));
				return (response);
			}
		}
		if (route.location->autoindex == true)
			return (autoindex(route.fsPath, route.location->path, route.server));
		return (Response::makeError(403, route.server));
	}
	return (Response::makeError(403, route.server));
}

Response	HttpHandler::handlePost(const Request &req, const RouteMatch &route) const
{
	struct stat	fileInfo;
	Response	response;
	std::string	path;
	std::size_t	limit;

	limit = bodyLimit(route);
	if (limit > 0 && req.body().size() > limit)
		return (Response::makeError(413, route.server));
	if (route.location->uploadStore.empty())
		return (Response::makeError(403, route.server));
	path = uploadFile(req, *route.location);
	if (path.empty())
		return (Response::makeError(403, route.server));
	if (stat(path.c_str(), &fileInfo) == 0)
	{
		if (!S_ISREG(fileInfo.st_mode))
			return (Response::makeError(S_ISDIR(fileInfo.st_mode) ? 409 : 403, route.server));
		std::ofstream fileWrite(path.c_str(), std::ios::binary);
		if (!fileWrite.is_open())
			return (Response::makeError(403, route.server));
		fileWrite.write(req.body().data(), req.body().size());
		if (!fileWrite)
			return (Response::makeError(500, route.server));
		fileWrite.close();
		if (!fileWrite)
			return (Response::makeError(500, route.server));
		response.setBody("File Uploaded");
		return (response);
	}
	if (errno != ENOENT)
	{
		if (errno == EACCES || errno == EPERM)
			return (Response::makeError(403, route.server));
		if (errno == ENOTDIR)
			return (Response::makeError(404, route.server));
		return (Response::makeError(500, route.server));
	}
	std::ofstream	file(path.c_str(), std::ios::binary);
	file.write(req.body().data(), req.body().size());
	if (!file)
		return (Response::makeError(500, route.server));
	file.close();
	if (!file)
		return (Response::makeError(500, route.server));
	response.setBody("File created");
	response.setStatus(201);
	return (response);
}

Response HttpHandler::handleDelete(const RouteMatch &route) const
{
	struct stat fileInfo;
	Response response;
	int result;

	if (stat(route.fsPath.c_str(), &fileInfo) == -1)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			return (Response::makeError(404, route.server));
		if (errno == EACCES || errno == EPERM)
			return (Response::makeError(403, route.server));
		return (Response::makeError(500, route.server));
	}
	if (S_ISDIR(fileInfo.st_mode))
		result = rmdir(route.fsPath.c_str());
	else
		result = unlink(route.fsPath.c_str());
	if (result == -1)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			return (Response::makeError(404, route.server));
		if (errno == EACCES || errno == EPERM)
			return (Response::makeError(403, route.server));
		if (errno == ENOTEMPTY || errno == EEXIST)
			return (Response::makeError(409, route.server));
		return (Response::makeError(500, route.server));
	}
	response.setStatus(204);
	return (response);
}

Response	HttpHandler::handleRedirect(const LocationConfig &loc) const
{
	Response	r;
	r.setStatus(loc.redirectCode ? loc.redirectCode : 302);
	r.setHeader("Location", loc.redirect);
	r.setBody("");
	return (r);
}

std::string	HttpHandler::escapeHTML(const std::string &str) const
{
	std::string	result;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == '&')
			result += "&amp;";
		else if (str[i] == '<')
			result += "&lt;";
		else if (str[i] == '>')
			result += "&gt;";
		else if (str[i] == '"')
			result += "&quot;";
		else if (str[i] == '\'')
			result += "&#39;";
		else
			result += str[i];
	}
	return (result);
}

std::string	HttpHandler::encodeURL(const std::string &str) const
{
	const char	*hex = "0123456789ABCDEF";
	std::string	result;

	for (size_t i = 0; i< str.size(); ++i)
	{
		unsigned char	c = str[i];
		if ((c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')
			|| c == '-' || c == '_'
			|| c == '.' || c == '~')
			result += c;
		else
		{
			result += '%';
			result += hex[c >> 4];
			result += hex[c & 15];
		}
	}
	return (result);
}

Response HttpHandler::autoindex(const std::string &fsPath,
                                const std::string &uri,
                                const ServerConfig *server) const
{
    DIR             *dir;
    struct dirent   *entry;
    std::string     body;
    std::string     name;
    std::string     href;
    Response        response;

    dir = opendir(fsPath.c_str());
    if (dir == NULL)
    {
        if (errno == EACCES)
            return (Response::makeError(403, server));
        return (Response::makeError(500, server));
    }
    body = "<html><body>";
    body += "<h1>Index of " + uri + "</h1>";
    body += "<ul>";
    while ((entry = readdir(dir)) != NULL)
    {
        name = entry->d_name;
        if (name == "." || name == "..")
            continue ;
        href = uri;
        if (href.empty() || href[href.size() - 1] != '/')
            href += "/";
        href += encodeURL(name);
        body += "<li><a href=\"";
        body += escapeHTML(href);
        body += "\">";
        body += escapeHTML(name);
        body += "</a></li>";
    }
    closedir(dir);
    body += "</ul>";
    body += "</body></html>";
    response.setStatus(200);
    response.setHeader("Content-Type", "text/html");
    response.setBody(body);
    return (response);
}

bool	HttpHandler::methodAllowed(const LocationConfig &loc, const std::string &m) const
{
	if (loc.allowedMethods.empty())
		return (m == "GET"); /* sensible default until config fills methods */
	for (std::size_t i = 0; i < loc.allowedMethods.size(); ++i)
		if (loc.allowedMethods[i] == m)
			return (true);
	return (false);
}
