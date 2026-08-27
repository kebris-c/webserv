/* **************************************************************************
 * Connection.cpp
 * OWNER: kebris-c
 * ************************************************************************** */

#include "Connection.hpp"

/* Creates an invalid connection with empty buffers and a fresh timeout. */
Connection::Connection()
	: _fd(-1), _state(CONN_READING_REQUEST), _bytesSent(0),
	  _lastActivity(std::time(0)), _stateStartedAt(_lastActivity),
	  _serverIndex(0), _remoteAddr("0.0.0.0"), _cgi(NULL)
{
}

/* Takes ownership of an accepted client fd and starts in the reading phase. */
Connection::Connection(int fd)
	: _fd(fd), _state(CONN_READING_REQUEST), _bytesSent(0),
	  _lastActivity(std::time(0)), _stateStartedAt(_lastActivity),
	  _serverIndex(0), _remoteAddr("0.0.0.0"), _cgi(NULL)
{
}

/* Takes an accepted fd and records which listener configuration owns it. */
Connection::Connection(int fd, std::size_t serverIndex)
	: _fd(fd), _state(CONN_READING_REQUEST), _bytesSent(0),
	  _lastActivity(std::time(0)), _stateStartedAt(_lastActivity),
	  _serverIndex(serverIndex), _remoteAddr("0.0.0.0"), _cgi(NULL)
{
}

/* Takes fd, config index, and peer address captured at accept time. */
Connection::Connection(int fd, std::size_t serverIndex,
						const std::string &remoteAddr)
	: _fd(fd), _state(CONN_READING_REQUEST), _bytesSent(0),
	  _lastActivity(std::time(0)), _stateStartedAt(_lastActivity),
	  _serverIndex(serverIndex), _remoteAddr(remoteAddr), _cgi(NULL)
{
}

/* Closes the client descriptor when Server removes this connection. */
Connection::~Connection()
{
	delete _cgi;
	closeClient();
}

/* Returns the owned client descriptor used by poll() and Server's map. */
int	Connection::fd() const { return (_fd); }

/* Closes only the client fd while a terminating CGI remains owned. */
void	Connection::closeClient()
{
	if (_fd >= 0)
	{
		::close(_fd);
		_fd = -1;
	}
}

/* Disables both directions without freeing the fd used as the map key. */
void	Connection::shutdownClient()
{
	if (_fd >= 0)
		::shutdown(_fd, SHUT_RDWR);
}

/* Returns the connection's current mutually exclusive processing phase. */
ConnectionState	Connection::state() const { return (_state); }

/* Replaces the phase and starts its absolute deadline on real transitions. */
void	Connection::setState(ConnectionState s)
{
	if (_state != s)
	{
		_state = s;
		_stateStartedAt = std::time(0);
	}
}

/* Exposes accumulated input to the future incremental HTTP parser. */
std::string	&Connection::readBuf() { return (_readBuf); }

/* Exposes output queued for readiness-driven partial sends. */
std::string	&Connection::writeBuf() { return (_writeBuf); }

/* Refreshes the idle timeout after successful network progress. */
void	Connection::touch() { _lastActivity = std::time(0); }

/* Returns true when no successful I/O occurred within timeoutSec seconds. */
bool	Connection::timedOut(std::time_t now, int timeoutSec) const
{
	return (now - _lastActivity >= timeoutSec);
}

/* Returns true once the current phase reaches timeoutSec. */
bool	Connection::phaseTimedOut(std::time_t now, int timeoutSec) const
{
	return (now - _stateStartedAt >= timeoutSec);
}

/* Returns the number of bytes already sent from the current write buffer. */
std::size_t	Connection::bytesSent() const { return (_bytesSent); }

/* Advances the partial-send offset by count bytes. */
void	Connection::addBytesSent(std::size_t count) { _bytesSent += count; }

/* Reports whether poll must continue monitoring this client for POLLOUT. */
bool	Connection::hasPendingWrite() const
{
	return (_bytesSent < _writeBuf.size());
}

/* Clears completed output and resets the offset for the next response. */
void	Connection::clearWrite()
{
	_writeBuf.clear();
	_bytesSent = 0;
}

/* Returns the incremental HTTP request associated with this connection. */
Request	&Connection::request() { return (_request); }

/* Returns the ServerConfig index selected when this client was accepted. */
std::size_t	Connection::serverIndex() const { return (_serverIndex); }

/* Returns peer IPv4 text for kmarrero's CGI REMOTE_ADDR environment entry. */
const std::string	&Connection::remoteAddr() const { return (_remoteAddr); }

/* Returns the active asynchronous CGI process, or NULL when absent. */
CgiProcess	*Connection::cgi() { return (_cgi); }

/* Transfers ownership of cgi into this connection. */
void	Connection::attachCgi(CgiProcess *cgi)
{
	if (_cgi != cgi)
	{
		delete _cgi;
		_cgi = cgi;
	}
}

/* Releases a completed or failed CGI process and its pipe descriptors. */
void	Connection::clearCgi()
{
	delete _cgi;
	_cgi = NULL;
}
