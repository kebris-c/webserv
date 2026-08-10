/* **************************************************************************
 * Response.hpp
 * OWNER: kmarrero
 * GOAL: Build HTTP responses (status, headers, body) into a byte string for send.
 * LEARN:
 *   - Accurate status codes (subject requirement)
 *   - Content-Length, Content-Type, Connection, Location (redirects)
 *   - Default error pages when config does not override
 * NOTES:
 *   - Server only writes Response::raw() bytes via poll POLLOUT.
 *   - Prefer building full response into memory first for static files;
 *     large files may later stream from disk (disk read OK without poll).
 * ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Webserv.hpp"

class Response {
public:
	Response();
	~Response();

	void	setStatus(int code);
	void	setHeader(const std::string &key, const std::string &value);
	void	setBody(const std::string &body);
	void	setBodyFromFile(const std::string &path);	/* TODO: read disk file */

	/* Build wire format: status-line + headers + CRLF + body */
	std::string	raw() const;

	static Response	makeError(int code, const std::string &pagePath = "");

private:
	int									_status;
	std::map<std::string, std::string>	_headers;
	std::string							_body;
};

#endif /* RESPONSE_HPP */
