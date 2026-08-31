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
	Socket();
	explicit Socket(int fd);
	~Socket();

	/* Create non-blocking listening socket for host:port. Returns false on error. */
	bool	listenOn(const std::string &host, int port);

	/* Accept one client; returns fd or -1 if would-block / error. */
	int		acceptClient() const;

	int		fd() const;
	void	setNonBlocking(int fd) const;
	void	closeFd();

private:
	int		_fd;

	Socket(const Socket &);
	Socket	&operator=(const Socket &);
};

#endif /* SOCKET_HPP */
