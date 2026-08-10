/* **************************************************************************
 * Server.hpp
 * OWNER: kebris-c
 * GOAL: Single process event loop multiplexing ALL listen + client (+ CGI pipe) fds.
 * LEARN:
 *   - Subject IV.1: one poll/epoll for all I/O between clients and server
 *   - Monitor reading AND writing simultaneously
 *   - Never read/write sockets/pipes without prior readiness
 *   - Disk files do NOT need poll readiness
 *   - Multi-port: several listen fds in the same loop
 *   - Stress: remain available; handle disconnects
 * HARD RULE: blocking I/O on sockets/pipes => project can be graded 0.
 * ============================================================================ */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Webserv.hpp"
#include "Config.hpp"
#include "Socket.hpp"
#include "Connection.hpp"

class Server {
public:
	Server();
	~Server();

	bool	configure(const std::vector<ServerConfig> &servers);
	int		run();		/* main loop until fatal error / signal */

private:
	std::vector<Socket *>				_listeners;
	std::map<int, Connection *>			_connections;
	std::vector<ServerConfig>			_configs;
	bool								_running;

	bool	_setupListeners();
	void	_acceptNew(int listenFd);
	void	_onReadable(int fd);
	void	_onWritable(int fd);
	void	_closeConnection(int fd);
	void	_checkTimeouts();

	/*
	 * PSEUDOCODE (event loop) — OWNER kebris-c — implement for real in Server.cpp
	 *
	 * build pollfd[] from: all listen fds (POLLIN)
	 *                    + each connection: POLLIN if reading / need body
	 *                                       POLLOUT if writeBuf non-empty
	 *                    + CGI pipe fds when active
	 * poll(fds, n, timeout_ms)
	 * for each ready fd:
	 *   if listen && POLLIN -> accept (non-blocking), register client
	 *   if client POLLIN    -> recv into readBuf; try parse Request (kmarrero)
	 *                         if complete -> HttpHandler -> fill writeBuf
	 *   if client POLLOUT   -> send from writeBuf; on done -> close or reset
	 *   if CGI pipe ready   -> move bytes; when CGI done -> finalize Response
	 * sweep timed-out connections
	 */
};

#endif /* SERVER_HPP */
