/* **************************************************************************
 * HttpHandler.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

#include "HttpHandler.hpp"

HttpHandler::HttpHandler() {}
HttpHandler::~HttpHandler() {}

Response	HttpHandler::handle(const Request &req, const RouteMatch &route) const
{
	/*
	 * TODO(kmarrero):
	 * - if !route.ok -> 404
	 * - if redirect configured -> _handleRedirect
	 * - if method not allowed -> 405
	 * - if CGI extension matches -> hand off to CgiProcess (kebris-c runs it)
	 * - dispatch GET/POST/DELETE
	 */
	(void)req;
	(void)route;
	return (Response::makeError(501));
}

Response	HttpHandler::_handleGet(const Request &req, const RouteMatch &route) const
{
	/* TODO: file / directory index / autoindex / 404 / 403 */
	(void)req;
	(void)route;
	return (Response::makeError(501));
}

Response	HttpHandler::_handlePost(const Request &req, const RouteMatch &route) const
{
	/*
	 * TODO(kmarrero): uploads — write body to location.uploadStore
	 * INVESTIGATE: multipart/form-data vs raw body; subject requires upload capability
	 * Enforce client_max_body_size (413).
	 */
	(void)req;
	(void)route;
	return (Response::makeError(501));
}

Response	HttpHandler::_handleDelete(const Request &req, const RouteMatch &route) const
{
	/* TODO(kmarrero): unlink file if allowed; careful with directories */
	(void)req;
	(void)route;
	return (Response::makeError(501));
}

Response	HttpHandler::_handleRedirect(const LocationConfig &loc) const
{
	Response	r;
	r.setStatus(loc.redirectCode ? loc.redirectCode : 302);
	r.setHeader("Location", loc.redirect);
	r.setBody("");
	return (r);
}

Response	HttpHandler::_autoindex(const std::string &fsPath, const std::string &uri) const
{
	/*
	 * TODO(kmarrero): opendir/readdir/closedir -> simple HTML listing
	 * INVESTIGATE: escape HTML in filenames; show hrefs relative to uri
	 */
	(void)fsPath;
	(void)uri;
	return (Response::makeError(501));
}

bool	HttpHandler::_methodAllowed(const LocationConfig &loc, const std::string &m) const
{
	if (loc.allowedMethods.empty())
		return (m == "GET"); /* sensible default until config fills methods */
	for (std::size_t i = 0; i < loc.allowedMethods.size(); ++i)
		if (loc.allowedMethods[i] == m)
			return (true);
	return (false);
}
