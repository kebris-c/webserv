/* **************************************************************************
 * Request.hpp
 * OWNER: kmarrero
 * GOAL: Parse HTTP request from Connection::readBuf into structured fields.
 * LEARN:
 *   - RFC 9112 request line + headers
 *   - Content-Length vs Transfer-Encoding: chunked (unchunk before CGI)
 *   - When is a request "complete"? (headers + body)
 *   - Reject bad requests with proper 400/411/413/405 later via Response
 * NOTES:
 *   - Parser should be incremental: may be called as bytes arrive.
 *   - Do not read from sockets here — only consume a string buffer.
 * ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "Webserv.hpp"

enum RequestParseState {
	REQ_LINE,
	REQ_HEADERS,
	REQ_BODY,
	REQ_COMPLETE,
	REQ_ERROR
};

class Request {
public:
	Request();
	~Request();

	void	reset();

	/* Feed available bytes; returns true when REQ_COMPLETE or REQ_ERROR. */
	bool	parse(std::string &buffer);

	RequestParseState				state() const;
	const std::string				&method() const;
	const std::string				&target() const;		/* path (+ query later) */
	const std::string				&query() const;
	const std::string				&version() const;
	const std::map<std::string, std::string>	&headers() const;
	const std::string				&body() const;
	int								errorCode() const;	/* if REQ_ERROR */

	/* Optional: case-insensitive header lookup. */
	std::string	header(const std::string &name) const;

private:
	RequestParseState				_state;
	std::string						_method;
	std::string						_target;
	std::string						_query;
	std::string						_version;
	std::map<std::string, std::string>	_headers;
	std::string						_body;
	std::size_t						_contentLength;
	bool							_chunked;
	int								_errorCode;

	/*
	 * PSEUDOCODE — OWNER kmarrero
	 *
	 * while state != COMPLETE/ERROR:
	 *   if REQ_LINE: need "\r\n" -> split METHOD SP TARGET SP HTTP/x.y
	 *                split TARGET into path ? query
	 *   if REQ_HEADERS: read lines until empty line
	 *                   detect Content-Length / Transfer-Encoding
	 *   if REQ_BODY:
	 *     if chunked: decode chunks until 0-size chunk (CGI needs unchunked body)
	 *     else: accumulate until Content-Length bytes
	 *     enforce client_max_body_size from matched server/location (Router later)
	 */
};

#endif /* REQUEST_HPP */
