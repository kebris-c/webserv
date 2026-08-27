/* **************************************************************************
 * HttpHandler.hpp
 * OWNER: kmarrero (HTTP semantics) — calls into CgiProcess owned by kebris-c
 * GOAL: Turn Request + RouteMatch into Response (static, upload, delete, redirect, CGI).
 * LEARN:
 *   - Serve fully static website
 *   - Directory index + autoindex HTML listing
 *   - POST upload to upload_store
 *   - DELETE resource
 *   - Method not allowed -> 405
 *   - Body too large -> 413
 * NOTES:
 *   - Keep this free of raw socket calls.
 *   - CGI: prepare env + args here; let CgiProcess run the child.
 * ************************************************************************** */

#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include "Webserv.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"

class HttpHandler {
public:
	HttpHandler();
	~HttpHandler();

	Response	handle(const Request &req, const RouteMatch &route) const;
	/*
	 * CGI decision for Server (kebris-c call site). Stub until you implement it.
	 * See workaround TODO in HttpHandler.cpp — keep signature stable.
	 */
	bool		prepareCgi(const Request &req, const RouteMatch &route,
						const std::string &remoteAddr,
						std::string &scriptPath,
						std::vector<std::string> &environment) const;
	/*
	 * CGI stdout -> Response after kebris-c finished pipes/reap.
	 * See workaround TODO in HttpHandler.cpp.
	 */
	Response	parseCgiOutput(const std::string &output) const;

private:
	Response	_handleGet(const Request &req, const RouteMatch &route) const;
	Response	_handlePost(const Request &req, const RouteMatch &route) const;
	Response	_handleDelete(const Request &req, const RouteMatch &route) const;
	Response	_handleRedirect(const LocationConfig &loc) const;
	Response	_autoindex(const std::string &fsPath, const std::string &uri) const;
	bool		_methodAllowed(const LocationConfig &loc, const std::string &m) const;
};

#endif /* HTTP_HANDLER_HPP */
