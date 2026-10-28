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

# include "Config.hpp"
# include "Webserv.hpp"
# include "RequestStateMachine.hpp"

class Request
{
	private:
		int									_errorCode;
		bool								_chunked;
		std::string							_method;
		std::string							_target;
		std::string							_query;
		std::string							_version;
		std::string							_body;
		std::size_t							_contentLength;
		RequestState						_state;
		RStateMachine						_stateMachine;
		RequestContext						_ctx;
		std::vector<ServerConfig>			_server;
		std::map<std::string, std::string>	_headers;
		void								configureStateMachine();
	public:
		Request();
		~Request();
		void										reset();
		void										print(RequestContext& ctx);
		void										setServer(std::vector<ServerConfig>& server);
		void										setCurrentState(RequestState state);
		void										setError(std::string message, RequestContext& ctx, RequestState state, int errorCode);
		void										setMethod(std::string& method);
		void										setTarget(std::string& target);
		void										setQuery(std::string& query);
		void										setVersion(std::string& version);
		void										setBody(std::string& body);
		void										setContentLength(std::size_t& length);
		void										setHeaders(std::string name, std::string value, RequestContext& ctx);
		void										setErrorCode(int& code);
		RequestState								state() const;
		const std::string							&method() const;
		const std::string							&target() const;		/* URL path, without query */
		const std::string							&query() const;
		const std::string							&version() const;
		const std::map<std::string, std::string>	&headers() const;
		const std::string							&body() const;
		const bool									&chunked() const;
		const size_t								&contentLength() const;
		int											errorCode() const;	/* if REQ_ERROR */
		bool										isComplete();
		bool										parse(std::string &buffer, RequestParser& parser);
		bool										feed(const std::string& data, RequestParser& parser);
		RequestState								getCurrentState();
		RequestState								getStateMachineState();
		RequestContext								getContext();
		/* Optional: case-insensitive header lookup. */
		std::string									header(const std::string &name) const;
};

#endif
