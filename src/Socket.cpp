/* **************************************************************************
 * Socket.cpp
 * OWNER: kebris-c
 * ************************************************************************** */

#include "Socket.hpp"

/* Creates an invalid socket owner; listenOn() assigns its descriptor. */
Socket::Socket() : _fd(-1) {}

/* Takes ownership of fd; the destructor will close it. */
Socket::Socket(int fd) : _fd(fd) {}

/* Releases the descriptor owned by this object. */
Socket::~Socket() { closeFd(); }

/*
 * Creates a non-blocking TCP listener for host:port.
 * Server calls this once per configured interface and port.
 */
bool	Socket::listenOn(const std::string &host, int port)
{
	struct addrinfo		hints;
	struct addrinfo		*addresses;
	struct addrinfo		*current;
	std::ostringstream	portStream;
	int					fd;
	int					reuseAddress;

	if (port <= 0 || port > 65535)
		return (false);
	closeFd();
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	portStream << port;
	addresses = NULL;
	if (::getaddrinfo(host.empty() ? NULL : host.c_str(),
			portStream.str().c_str(), &hints, &addresses) != 0)
		return (false);
	fd = -1;
	reuseAddress = 1;
	for (current = addresses; current != NULL; current = current->ai_next)
	{
		fd = ::socket(current->ai_family, current->ai_socktype,
				current->ai_protocol);
		if (fd < 0)
			continue;
		if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseAddress,
				sizeof(reuseAddress)) == 0
			&& ::bind(fd, current->ai_addr, current->ai_addrlen) == 0)
			break;
		::close(fd);
		fd = -1;
	}
	::freeaddrinfo(addresses);
	if (fd < 0)
		return (false);
	if (::listen(fd, 128) < 0 || !setNonBlocking(fd))
	{
		::close(fd);
		return (false);
	}
	_fd = fd;
	return (true);
}

/*
 * Accepts one client from this listener after poll reports POLLIN.
 * Fills remoteAddr with dotted IPv4 for CGI REMOTE_ADDR (whitelist-safe).
 * Server owns the returned descriptor through a Connection.
 */
int	Socket::acceptClient(std::string &remoteAddr) const
{
	struct sockaddr_storage	peer;
	socklen_t				peerLen;
	int						clientFd;
	unsigned long			address;
	std::ostringstream		stream;

	peerLen = sizeof(peer);
	std::memset(&peer, 0, sizeof(peer));
	remoteAddr = "0.0.0.0";
	clientFd = ::accept(_fd, reinterpret_cast<struct sockaddr *>(&peer),
			&peerLen);
	if (clientFd < 0)
		return (-1);
	if (peer.ss_family == AF_INET)
	{
		address = ntohl(reinterpret_cast<struct sockaddr_in *>(&peer)
				->sin_addr.s_addr);
		stream << ((address >> 24) & 0xff) << '.'
			   << ((address >> 16) & 0xff) << '.'
			   << ((address >> 8) & 0xff) << '.'
			   << (address & 0xff);
		remoteAddr = stream.str();
	}
	if (!setNonBlocking(clientFd))
	{
		::close(clientFd);
		return (-1);
	}
	return (clientFd);
}

/* Returns the listener descriptor so Server can register it in poll(). */
int	Socket::fd() const { return (_fd); }

/* Enables non-blocking mode using only subject-authorized fcntl flags. */
bool	Socket::setNonBlocking(int fd) const
{
	return (fd >= 0 && ::fcntl(fd, F_SETFL, O_NONBLOCK) != -1);
}

/* Closes the owned descriptor once and marks this object as empty. */
void	Socket::closeFd()
{
	if (_fd >= 0)
	{
		::close(_fd);
		_fd = -1;
	}
}
