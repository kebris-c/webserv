/* **************************************************************************
 * Connection.cpp
 * OWNER: kebris-c
 * ************************************************************************** */

#include "Connection.hpp"

Connection::Connection()
	: _fd(-1), _state(CONN_READING_REQUEST), _bytesSent(0), _lastActivity(std::time(0))
{
}

Connection::Connection(int fd)
	: _fd(fd), _state(CONN_READING_REQUEST), _bytesSent(0), _lastActivity(std::time(0))
{
}

Connection::~Connection()
{
	if (_fd >= 0)
	{
		::close(_fd);
		_fd = -1;
	}
}

int	Connection::fd() const { return (_fd); }
ConnectionState	Connection::state() const { return (_state); }
void	Connection::setState(ConnectionState s) { _state = s; }

std::string	&Connection::readBuf() { return (_readBuf); }
std::string	&Connection::writeBuf() { return (_writeBuf); }

void	Connection::touch() { _lastActivity = std::time(0); }

bool	Connection::timedOut(std::time_t now, int timeoutSec) const
{
	return (now - _lastActivity >= timeoutSec);
}
