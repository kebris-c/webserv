/* **************************************************************************
 * Connection.hpp
 * OWNER: kebris-c (buffers / lifecycle) — Request parse hooks used by kmarrero
 * GOAL: Per-client state: readBuf, writeBuf, timeout, parse/response phases.
 * LEARN:
 *   - Partial recv/send loops driven by POLLIN/POLLOUT
 *   - When to switch interest from read to write in poll
 *   - Detect peer close (recv == 0) and errors without errno-driven recovery
 *     after read/write (subject forbids using errno to adjust behaviour after
 *     read/write — design readiness + return values carefully)
 *   - Request timeout so clients never hang forever
 * ============================================================================ */

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "Webserv.hpp"
#include "Request.hpp"
#include "CgiProcess.hpp"

enum ConnectionState {
	CONN_READING_REQUEST,
	CONN_PROCESSING,		/* handler / CGI in progress */
	CONN_WRITING_RESPONSE,
	CONN_CLOSING
};

class Connection {
public:
	/* Creates an invalid connection; kept for simple C++98 construction. */
	Connection();
	/* Takes ownership of a new client fd accepted by Server. */
	explicit Connection(int fd);
	/* Also records which listener configuration accepted this client. */
	Connection(int fd, std::size_t serverIndex);
	/* Records peer address text for CGI REMOTE_ADDR construction. */
	Connection(int fd, std::size_t serverIndex, const std::string &remoteAddr);
	/* Closes the owned client fd when Server removes the connection. */
	~Connection();

	/* Returns the client descriptor used as the connection map key. */
	int					fd() const;
	/* Closes and invalidates the client descriptor during final cleanup. */
	void				closeClient();
	/* Disables client traffic while retaining its fd key during CGI reap. */
	void				shutdownClient();
	/* Returns the current mutually exclusive connection phase. */
	ConnectionState		state() const;
	/* Changes the phase when input ends or processing progresses. */
	void				setState(ConnectionState s);

	/* Returns buffered input; the HTTP parser will consume it later. */
	std::string			&readBuf();
	/* Returns buffered output; Server sends it only after POLLOUT. */
	std::string			&writeBuf();

	/* Records successful I/O; Server calls it after recv/send progress. */
	void				touch();
	/* Reports idle expiration during Server's periodic timeout sweep. */
	bool				timedOut(std::time_t now, int timeoutSec) const;
	/* Reports a phase deadline even when a peer or CGI keeps trickling data. */
	bool				phaseTimedOut(std::time_t now, int timeoutSec) const;

	/* Returns the offset already sent from writeBuf(). */
	std::size_t			bytesSent() const;
	/* Advances the output offset after a successful partial send. */
	void				addBytesSent(std::size_t count);
	/* Reports whether writeBuf still has unsent bytes. */
	bool				hasPendingWrite() const;
	/* Drops fully sent output and resets its offset for reuse. */
	void				clearWrite();
	/* Returns the incremental request object owned by this client. */
	Request				&request();
	/* Returns the configuration index selected by the accepting listener. */
	std::size_t			serverIndex() const;
	/* Returns dotted IPv4 peer text for HttpHandler/CGI env (REMOTE_ADDR). */
	const std::string	&remoteAddr() const;
	/* Returns the active CGI process, or NULL outside CGI processing. */
	CgiProcess			*cgi();
	/* Transfers one newly started CGI process into this connection. */
	void				attachCgi(CgiProcess *cgi);
	/* Deletes the completed or failed CGI process. */
	void				clearCgi();

private:
	int					_fd;
	ConnectionState		_state;
	std::string			_readBuf;
	std::string			_writeBuf;
	std::size_t			_bytesSent;
	std::time_t			_lastActivity;
	std::time_t			_stateStartedAt;
	Request				_request;
	std::size_t			_serverIndex;
	std::string			_remoteAddr;
	CgiProcess			*_cgi;

	Connection(const Connection &);
	Connection	&operator=(const Connection &);
};

#endif /* CONNECTION_HPP */
