/* **************************************************************************
 * Socket.cpp
 * OWNER: kebris-c
 * ************************************************************************** */

#include "Socket.hpp"

Socket::Socket() : _fd(-1) {}
Socket::Socket(int fd) : _fd(fd) {}
Socket::~Socket() { closeFd(); }

bool	Socket::listenOn(const std::string &host, int port)
{
	/*
	 * TODO(kebris-c):
	 * - getaddrinfo(host, port)
	 * - socket(); setsockopt(SO_REUSEADDR)
	 * - bind(); listen()
	 * - setNonBlocking(_fd)
	 * INVESTIGATE: backlog size; IPv4 vs IPv6; why SO_REUSEADDR for restarts
	 */
	(void)host;
	(void)port;
	return (false);
}

int	Socket::acceptClient() const
{
	/*
	 * TODO(kebris-c): accept(); setNonBlocking(clientFd); return fd
	 * On EAGAIN/EWOULDBLOCK style conditions, return -1 without treating as fatal.
	 * Remember subject: do not use errno after read/write to adjust behaviour —
	 * accept is not read/write, but still keep the loop non-blocking.
	 */
	return (-1);
}

int	Socket::fd() const { return (_fd); }

void	Socket::setNonBlocking(int fd) const
{
	/*
	 * TODO(kebris-c): fcntl(fd, F_SETFL, O_NONBLOCK)
	 * macOS: only F_SETFL, O_NONBLOCK, FD_CLOEXEC allowed if you need CLOEXEC.
	 */
	(void)fd;
}

void	Socket::closeFd()
{
	if (_fd >= 0)
	{
		::close(_fd);
		_fd = -1;
	}
}
