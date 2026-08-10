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

enum ConnectionState {
	CONN_READING_REQUEST,
	CONN_PROCESSING,		/* handler / CGI in progress */
	CONN_WRITING_RESPONSE,
	CONN_CLOSING
};

class Connection {
public:
	Connection();
	explicit Connection(int fd);
	~Connection();

	int					fd() const;
	ConnectionState		state() const;
	void				setState(ConnectionState s);

	std::string			&readBuf();
	std::string			&writeBuf();

	void				touch();					/* update last activity */
	bool				timedOut(std::time_t now, int timeoutSec) const;

	/* TODO(kebris-c): append from recv; consume from send; track bytesSent. */

private:
	int					_fd;
	ConnectionState		_state;
	std::string			_readBuf;
	std::string			_writeBuf;
	std::size_t			_bytesSent;
	std::time_t			_lastActivity;
};

#endif /* CONNECTION_HPP */
