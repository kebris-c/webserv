/* **************************************************************************
 * HttpHandler.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

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
	// ============================================================
	// TODO: This is a workaround to allow me keep going on, must be
	// changed/improved before the project end.
	// WHY (kebris-c): Server needs a CGI decision + path/env WITHOUT owning
	// HTTP/route semantics; this API was added on your HttpHandler so the
	// poll/CGI process plane could compile and run.
	// YOU (kmarrero): if this request is CGI, set scriptPath, fill environment
	// (KEY=VALUE, include REMOTE_ADDR=remoteAddr via buildEnv), return true.
	// Otherwise return false and Server will call handle() instead.
	// RESPECT: keep this signature (incl. remoteAddr); do NOT recv/send/fork/
	// pipe/poll here; body must already be unchunked in Request before true;
	// kebris-c owns CgiProcess::start + pipe I/O + waitpid(WNOHANG).
	// ============================================================
	(void)req;
	(void)route;
	(void)remoteAddr;
	scriptPath.clear();
	environment.clear();
	return (false);
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
			std::ofstream fileWrite(route.fsPath.c_str(), std::ios::binary | std::ios::app);
			if (!fileWrite.is_open())
				return (Response::makeError(403));
			fileWrite.write(req.body().data(), req.body().size());
			if (!fileWrite)
				return (Response::makeError(500));
			fileWrite.close();
			response.setBody("File Uploaded");
			return (response);
		}
	}
	else if (errno == ENOENT)
	{
		std::ofstream	file(route.fsPath.c_str(), std::ios::binary);
		file.write(req.body().data(), req.body().size());
		if (!file)
			return (Response::makeError(500));
		file.close();
		response.setBody("File created");
		response.setStatus(200);
		return (response);
	}
	return (Response::makeError(501));
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
