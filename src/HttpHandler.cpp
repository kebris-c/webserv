/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 16:45:17 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/28 19:29:56 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiProcess.hpp"
#include "HttpHandler.hpp"

HttpHandler::HttpHandler()
{}

HttpHandler::~HttpHandler()
{}

Response	HttpHandler::handle(const Request &req, const RouteMatch &route) const
{
	const LocationConfig	&loc = *(route).location;
	std::string				method;

	if (!methodAllowed(loc, req.method()))
		return (Response::makeError(405));
	method = req.method();
	if (method == "GET")
		return (handleGet(route));
	if (method == "POST")
		return (handlePost(req, route));
	if (method == "DELETE")
		return (handleDelete(route));
	return (Response::makeError(501));
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
	const LocationConfig	&loc = *route.location;
	std::string				path = route.fsPath;

	if (!route.ok || route.server == NULL || route.location == NULL
		|| req.state() != REQ_COMPLETE || route.fsPath.empty())
		return (false);
	environment.clear();
	scriptPath.clear();
	if (loc.cgiPass.empty() || loc.cgiExtension.empty()
		|| !methodAllowed(loc, req.method())
		|| (req.method() != "GET" && req.method() != "POST")
		|| req.body().size() > route.server->clientMaxBodySize
		|| route.fsPath.size () < loc.cgiPass.size()
		|| route.fsPath.compare(route.fsPath.size() - loc.cgiExtension.size(),
				loc.cgiExtension.size(), loc.cgiExtension) != 0)
		return (false);
	if (path[0] != '/')
	{
		char	cwd[PATH_MAX];
	
		if (getcwd(cwd, sizeof(cwd)) == NULL)
			throw std::runtime_error("Cannot resolved CGI script path");
		path = std::string(cwd) + "/" + path;
	}
	environment = CgiProcess::buildEnv(req, *route.server, loc, path);
	environment.push_back("REMOTE_ADDR=" + remoteAddr);
	scriptPath.swap(path);
	return (true);
}

/*
 * Converts complete CGI stdout into an HTTP response after pipe EOF.
 */
Response	HttpHandler::parseCgiOutput(const std::string &output) const
{
	// ============================================================
	// TODO: This is a workaround to allow me keep going on, must be
	// changed/improved before the project end.
	// WHY (kebris-c): Server collects raw CGI stdout asynchronously, then needs
	// YOUR conversion to Response; stub returns 501 so the call site exists.
	// YOU (kmarrero): parse CGI headers until blank line; honour Status: if
	// present else default 200; body = remainder (EOF already means end if no
	// Content-Length). Return a complete Response (raw()-ready).
	// RESPECT: no socket/pipe I/O here; input is the full buffer from
	// CgiProcess::output() after kebris-c closed stdout and reaped the child.
	// ============================================================
	(void)output;
	return (Response::makeError(501));
}

Response	HttpHandler::handleGet(const RouteMatch &route) const
{
	struct stat		fileInfo;
	Response		response;

	if (lstat(route.fsPath.c_str(), &fileInfo) == -1)
	{
		if (errno == EACCES)
			return (Response::makeError(403));
		if (errno == ENOENT || errno == ENOTDIR)
			return (Response::makeError(404));
		return (Response::makeError(500));
	}
	if (S_ISREG(fileInfo.st_mode))
	{
		response.setBodyFromFile(route.fsPath);
		return (response);
	}
	if (S_ISDIR(fileInfo.st_mode))
	{
		if (stat((route.fsPath + "/index.html").c_str(), &fileInfo) == 0
			&& S_ISREG(fileInfo.st_mode))
		{
			response.setBodyFromFile(route.fsPath + "/index.html");
			return (response);
		}
		if (route.location->autoindex == true)
			return (autoindex(route.fsPath, route.location->path));
		return (Response::makeError(403));
	}
	return (Response::makeError(403));
}

Response	HttpHandler::handlePost(const Request &req, const RouteMatch &route) const
{
	struct stat	fileInfo;
	Response	response;

	if (req.body().size() > route.server->clientMaxBodySize)
		return (Response::makeError(413));
	if (lstat(route.fsPath.c_str(), &fileInfo) == 0)
	{
		if (S_ISREG(fileInfo.st_mode))
		{
			std::ofstream fileWrite(route.fsPath.c_str(), std::ios::binary);
			if (!fileWrite.is_open())
				return (Response::makeError(403));
			fileWrite.write(req.body().data(), req.body().size());
			if (!fileWrite)
				return (Response::makeError(500));
			fileWrite.close();
			if (!fileWrite)
				return (Response::makeError(500));
			response.setBody("File Uploaded");
			return (response);
		}
		return (Response::makeError(S_ISDIR(fileInfo.st_mode) ? 409 : 403));
	}
	else if (errno == ENOENT)
	{
		std::ofstream	file(route.fsPath.c_str(), std::ios::binary);
		file.write(req.body().data(), req.body().size());
		if (!file)
			return (Response::makeError(500));
		file.close();
		if (!file)
			return (Response::makeError(500));
		response.setBody("File created");
		response.setStatus(201);
		return (response);
	}
	if (errno == EACCES || errno == EPERM)
		return (Response::makeError(403));
	if (errno == ENOTDIR)
		return (Response::makeError(404));
	return (Response::makeError(500));
}

Response HttpHandler::handleDelete(const RouteMatch &route) const
{
	struct stat fileInfo;
	Response response;
	int result;

	if (lstat(route.fsPath.c_str(), &fileInfo) == -1)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			return (Response::makeError(404));
		if (errno == EACCES || errno == EPERM)
			return (Response::makeError(403));
		return (Response::makeError(500));
	}
	if (S_ISDIR(fileInfo.st_mode))
		result = rmdir(route.fsPath.c_str());
	else
		result = unlink(route.fsPath.c_str());
	if (result == -1)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			return (Response::makeError(404));
		if (errno == EACCES || errno == EPERM)
			return (Response::makeError(403));
		if (errno == ENOTEMPTY || errno == EEXIST)
			return (Response::makeError(409));
		return (Response::makeError(500));
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
                                const std::string &uri) const
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
            return (Response::makeError(403));
        return (Response::makeError(500));
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
