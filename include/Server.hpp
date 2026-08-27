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
	/* Creates a stopped server with no listeners or clients. */
	Server();
	/* Releases every listener and active client descriptor. */
	~Server();

	/* Copies public config data and creates all configured listeners. */
	bool	configure(const std::vector<ServerConfig> &servers);
	/* Enables temporary TCP echo while kmarrero's HTTP plane is unavailable.
	 * Workaround: remove call from main once Config::load works (see main.cpp). */
	void	enableEchoWorkaround();
	/* Runs the single readiness loop until a fatal poll/listener error. */
	int		run();

private:
	std::vector<Socket *>				_listeners;
	std::map<int, std::size_t>			_listenerConfigs;
	std::map<int, Connection *>			_connections;
	std::vector<ServerConfig>			_configs;
	bool								_running;
	bool								_echoWorkaround;
	std::time_t							_acceptPausedUntil;

	bool	_setupListeners();
	void	_acceptNew(int listenFd);
	void	_onReadable(int fd);
	void	_onWritable(int fd);
	bool	_onCgiEvent(int fd, short revents);
	void	_checkCgiProcesses();
	void	_closeConnection(int fd);
	void	_checkTimeouts();

	Server(const Server &);
	Server	&operator=(const Server &);

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
