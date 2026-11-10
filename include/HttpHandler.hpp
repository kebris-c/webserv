/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 16:43:56 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/31 16:44:21 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_HANDLER_HPP
# define HTTP_HANDLER_HPP

#include "Webserv.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"

enum Method
{
	GET,
	POST,
	DELETE
};

class HttpHandler {
	public:
		HttpHandler();
		~HttpHandler();

		Response	handle(const Request &req, const RouteMatch &route) const;
		/*
		* CGI decision for Server without socket or pipe I/O.
		*/
		bool		prepareCgi(const Request &req, const RouteMatch &route,
							const std::string &remoteAddr,
							std::string &scriptPath,
							std::vector<std::string> &environment) const;
		/*
		* Complete CGI stdout -> Response after pipe EOF and child reaping.
		*/
		Response	parseCgiOutput(const std::string &output) const;
	private:
		Response	handleGet(const RouteMatch &route) const;
		Response	handlePost(const Request &req, const RouteMatch &route) const;
		Response	handleDelete(const RouteMatch &route) const;
		Response	handleRedirect(const LocationConfig &loc) const;
		Response	autoindex(const std::string &fsPath, const std::string &uri) const;
		bool		methodAllowed(const LocationConfig &loc, const std::string &m) const;
		std::string	escapeHTML(const std::string &str) const;
		std::string	encodeURL(const std::string &str) const;
};

#endif
