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
	/* Runs the single readiness loop until a fatal poll/listener error. */
	int		run();

private:
	std::vector<Socket *>				_listeners;
	std::map<int, std::size_t>			_listenerConfigs;
	std::map<int, Connection *>			_connections;
	std::vector<ServerConfig>			_configs;
	bool								_running;
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
};

#endif /* SERVER_HPP */
