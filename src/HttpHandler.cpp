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
