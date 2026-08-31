/* **************************************************************************
 * Server.cpp
 * OWNER: kebris-c
 * This is the hardest mandatory piece. Implement using the pseudocode in the header.
 * ************************************************************************** */

#include "Server.hpp"

Server::Server() : _running(false) {}

Server::~Server()
{
	for (std::map<int, Connection *>::iterator it = _connections.begin();
		 it != _connections.end(); ++it)
		delete it->second;
	_connections.clear();
	for (std::size_t i = 0; i < _listeners.size(); ++i)
		delete _listeners[i];
	_listeners.clear();
}

bool	Server::configure(const std::vector<ServerConfig> &servers)
{
	_configs = servers;
	return (_setupListeners());
}

bool	Server::_setupListeners()
{
	/*
	 * TODO(kebris-c): for each ServerConfig, Socket::listenOn(host, port)
	 * Store mapping listenFd -> ServerConfig index for later routing.
	 */
	if (_configs.empty())
		return (false);
	/* Skeleton: not listening yet */
	return (false);
}

int	Server::run()
{
	/*
	 * TODO(kebris-c): implement single poll/epoll loop (see Server.hpp PSEUDOCODE).
	 * INVESTIGATE:
	 *   man poll / man epoll
	 *   how nginx / your Python chat differ (threading vs multiplexing)
	 *   partial writes and when to enable POLLOUT
	 *   integrating CGI pipe fds into THIS same poll set
	 */
	_running = false;
	std::cerr << "Server::run stub — kebris-c must implement the event loop"
			  << std::endl;
	return (1);
}

void	Server::_acceptNew(int listenFd)
{
	(void)listenFd;
	/* TODO(kebris-c) */
}

void	Server::_onReadable(int fd)
{
	/*
	 * TODO(kebris-c): recv -> connection.readBuf()
	 * Then call kmarrero Request::parse.
	 * If complete, Router + HttpHandler (kmarrero) fill writeBuf / start CGI.
	 */
	(void)fd;
}

void	Server::_onWritable(int fd)
{
	/* TODO(kebris-c): send from writeBuf; advance offset; close or keep-alive */
	(void)fd;
}

void	Server::_closeConnection(int fd)
{
	std::map<int, Connection *>::iterator it = _connections.find(fd);
	if (it != _connections.end())
	{
		delete it->second;
		_connections.erase(it);
	}
}

void	Server::_checkTimeouts()
{
	/* TODO(kebris-c): close idle clients; subject: request must never hang forever */
}
