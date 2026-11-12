#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "Webserv.hpp"

class Socket {
public:
	/* Creates an empty socket owner; used by Server for each listener. */
	Socket();
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
