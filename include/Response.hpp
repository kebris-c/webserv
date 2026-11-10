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

struct ServerConfig;

class Response {
public:
	Response();
	~Response();

	void	setStatus(int code);
	void	setHeader(const std::string &key, const std::string &value);
	/* Append a separate field, e.g. a second Set-Cookie. */
	void	addHeader(const std::string &key, const std::string &value);
	void	setBody(const std::string &body);
	/* Reads a regular file. False when the file cannot be opened. */
	bool	setBodyFromFile(const std::string &path);

	/* Build wire format: status-line + headers + CRLF + body */
	std::string	raw() const;

	/* Uses server->errorPages when the file can be read; otherwise built-in HTML. */
	static Response	makeError(int code, const ServerConfig *server = NULL);

private:
	int									_status;
	std::multimap<std::string, std::string>	_headers;
	std::string							_body;
	std::map<std::string, std::string>	_extensionDispatcher;
	std::string	getExtension(const std::string& path);
};

#endif /* RESPONSE_HPP */
