/* **************************************************************************
 * Socket.hpp
 * OWNER: kebris-c
 * GOAL: Thin RAII-ish wrapper around listen/client socket fds (C++98: no real move).
 * LEARN:
 *   - socket, bind, listen, accept, setsockopt(SO_REUSEADDR)
 *   - getaddrinfo / freeaddrinfo for host:port
 *   - fcntl O_NONBLOCK (required behaviour; on macOS subject restricts fcntl flags)
 *   - NEVER block in accept/recv/send — readiness comes from Server poll loop
 * NOTES:
 *   - One listen socket per ServerConfig listen pair.
 *   - Closing policy: Server/Connection own lifetime; avoid double-close.
 * ============================================================================ */

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "Webserv.hpp"

class Socket {
public:
	/* Creates an empty socket owner; used by Server for each listener. */
	Socket();
	/* Takes ownership of fd; used only when an existing descriptor is wrapped. */
	explicit Socket(int fd);
	/* Closes the owned descriptor during listener cleanup. */
	~Socket();

	/* Creates a non-blocking listener for host:port; Server calls it at setup. */
	bool	listenOn(const std::string &host, int port);

	/* Accepts one non-blocking client after POLLIN; fills peer IPv4 text. */
	int		acceptClient(std::string &remoteAddr) const;

	/* Returns the owned listener descriptor; Server uses it in poll(). */
	int		fd() const;
	/* Sets O_NONBLOCK on fd; listener setup and accept use it. */
	bool	setNonBlocking(int fd) const;
	/* Closes and invalidates the owned descriptor; destructor also calls it. */
	void	closeFd();

private:
	int		_fd;

	Socket(const Socket &);
	Socket	&operator=(const Socket &);
};

#endif /* SOCKET_HPP */
