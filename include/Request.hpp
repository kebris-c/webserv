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
# define REQUEST_HPP

# include "Webserv.hpp"
# include "RStateMachine.hpp"

class Request {
	private:
		RequestState						_state;
		std::string							_method;
		std::string							_target;
		std::string							_query;
		std::string							_version;
		std::string							_body;
		std::size_t							_contentLength;
		std::map<std::string, std::string>	_headers;
		bool								_chunked;
		int									_errorCode;
		std::vector<std::string>			_chunkedBody;
	public:
		Request();
		~Request();
		void										reset();
		bool										parse(std::string &buffer);
		void										setCurrentState(RequestState state);
		void										setError(std::string message, RequestContext& ctx, RequestState state, int errorCode);
		void										setMethod(std::string& method);
		void										setTarget(std::string& target);
		void										setQuery(std::string& query);
		void										setVersion(std::string& version);
		void										setBody(std::string& body);
		void										setContentLength(std::size_t& length);
		void										setHeaders(std::string name, std::string value, RequestContext& ctx);
		void										setChunked(bool& answer);
		void										setErrorCode(int& code);
		void										setChunkedBody(std::vector<std::string>& chunk);
		RequestState								state() const;
		const std::string							&method() const;
		const std::string							&target() const;		/* path (+ query later) */
		const std::string							&query() const;
		const std::string							&version() const;
		const std::map<std::string, std::string>	&headers() const;
		const std::string							&body() const;
		int											errorCode() const;	/* if REQ_ERROR */
		void										feed(const std::string& data, RequestContext& ctx);
		void										print(RequestContext& ctx);
		RequestState								getCurrentState();
		/* Optional: case-insensitive header lookup. */
		std::string									header(const std::string &name) const;

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
