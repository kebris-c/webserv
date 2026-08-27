/* **************************************************************************
 * Server.cpp
 * OWNER: kebris-c
 * This is the hardest mandatory piece. Implement using the pseudocode in the header.
 * ************************************************************************** */

#include "Server.hpp"
#include "Router.hpp"
#include "HttpHandler.hpp"

namespace {

const std::size_t	MAX_PENDING_WRITE = 1024 * 1024;
const std::size_t	MAX_READ_BUFFER = 16 * 1024 * 1024;
const std::size_t	MAX_CONNECTIONS = 1024;
const int			POLL_TIMEOUT_MS = 500;
const int			MAX_PHASE_DURATION_SEC = 120;
volatile sig_atomic_t	g_stopRequested = 0;

/* Records only a stop request; Server::run performs cleanup outside the signal. */
void	requestStop(int)
{
	g_stopRequested = 1;
}

}

/* Creates a stopped server; configure() populates its listeners. */
Server::Server()
	: _running(false), _echoWorkaround(false), _acceptPausedUntil(0) {}

/* Releases all client and listener descriptors owned by the server. */
Server::~Server()
{
	for (std::map<int, Connection *>::iterator it = _connections.begin();
		 it != _connections.end(); ++it)
		delete it->second;
	_connections.clear();
	for (std::size_t i = 0; i < _listeners.size(); ++i)
		delete _listeners[i];
	_listeners.clear();
	_listenerConfigs.clear();
}

/* Copies public server configurations and creates their listen sockets. */
bool	Server::configure(const std::vector<ServerConfig> &servers)
{
	if (_running)
		return (false);
	_configs = servers;
	return (_setupListeners());
}

/* Enables only the explicitly marked temporary echo path in _onReadable(). */
void	Server::enableEchoWorkaround()
{
	_echoWorkaround = true;
}

/*
 * Builds one non-blocking listener per configured host:port.
 * configure() calls it before the event loop starts.
 */
bool	Server::_setupListeners()
{
	Socket	*listener;

	for (std::size_t i = 0; i < _listeners.size(); ++i)
		delete _listeners[i];
	_listeners.clear();
	_listenerConfigs.clear();
	if (_configs.empty())
		return (false);
	for (std::size_t i = 0; i < _configs.size(); ++i)
	{
		listener = NULL;
		try
		{
			listener = new Socket();
			if (!listener->listenOn(_configs[i].host, _configs[i].port))
				throw 0;
			_listenerConfigs[listener->fd()] = i;
			_listeners.push_back(listener);
			listener = NULL;
		}
		catch (...)
		{
			delete listener;
			for (std::size_t j = 0; j < _listeners.size(); ++j)
				delete _listeners[j];
			_listeners.clear();
			_listenerConfigs.clear();
			return (false);
		}
	}
	return (true);
}

/*
 * Runs the only poll() used for network I/O.
 * It monitors every listener and client for read and write readiness.
 */
int	Server::run()
{
	std::vector<struct pollfd>	pollFds;
	struct pollfd				entry;
	int							ready;

	if (_listeners.empty())
		return (1);
	g_stopRequested = 0;
	::signal(SIGPIPE, SIG_IGN);
	::signal(SIGINT, requestStop);
	::signal(SIGTERM, requestStop);
	_running = true;
	try
	{
		pollFds.reserve(_listeners.size() + MAX_CONNECTIONS * 3);
		while (_running && !g_stopRequested)
		{
			pollFds.clear();
			for (std::size_t i = 0; i < _listeners.size(); ++i)
			{
				entry.fd = _listeners[i]->fd();
				entry.events = 0;
				if (_connections.size() < MAX_CONNECTIONS
					&& std::time(0) >= _acceptPausedUntil)
					entry.events = POLLIN;
				entry.revents = 0;
				pollFds.push_back(entry);
			}
			for (std::map<int, Connection *>::iterator it = _connections.begin();
				 it != _connections.end(); ++it)
			{
				if (it->second->fd() >= 0
					&& !(it->second->state() == CONN_CLOSING
						&& it->second->cgi() != NULL
						&& it->second->cgi()->childActive()))
				{
					entry.fd = it->second->fd();
					entry.events = 0;
					if (it->second->state() == CONN_READING_REQUEST
						&& it->second->writeBuf().size()
							- it->second->bytesSent() < MAX_PENDING_WRITE)
						entry.events |= POLLIN;
					if (it->second->hasPendingWrite())
						entry.events |= POLLOUT;
					entry.revents = 0;
					pollFds.push_back(entry);
				}
				if (it->second->cgi() != NULL
					&& it->second->cgi()->state() == CGI_RUNNING)
				{
					if (it->second->cgi()->stdinFd() >= 0)
					{
						entry.fd = it->second->cgi()->stdinFd();
						entry.events = POLLOUT;
						entry.revents = 0;
						pollFds.push_back(entry);
					}
					if (it->second->cgi()->stdoutFd() >= 0)
					{
						entry.fd = it->second->cgi()->stdoutFd();
						entry.events = POLLIN;
						entry.revents = 0;
						pollFds.push_back(entry);
					}
				}
			}
			ready = ::poll(&pollFds[0], pollFds.size(), POLL_TIMEOUT_MS);
			if (ready < 0)
			{
				if (errno == EINTR)
					continue;
				_running = false;
				break;
			}
			for (std::size_t i = 0; i < pollFds.size() && ready > 0; ++i)
			{
				if (pollFds[i].revents == 0)
					continue;
				--ready;
				if (_listenerConfigs.find(pollFds[i].fd)
					!= _listenerConfigs.end())
				{
					if (pollFds[i].revents & POLLIN)
						_acceptNew(pollFds[i].fd);
					if (pollFds[i].revents & (POLLERR | POLLNVAL))
						_running = false;
					continue;
				}
				if (_onCgiEvent(pollFds[i].fd, pollFds[i].revents))
					continue;
				if (_connections.find(pollFds[i].fd) == _connections.end())
					continue;
				if (pollFds[i].revents & (POLLERR | POLLNVAL))
				{
					_closeConnection(pollFds[i].fd);
					continue;
				}
				if (pollFds[i].revents & POLLIN)
					_onReadable(pollFds[i].fd);
				if (_connections.find(pollFds[i].fd) == _connections.end())
					continue;
				if (pollFds[i].revents & POLLOUT)
					_onWritable(pollFds[i].fd);
				if (_connections.find(pollFds[i].fd) == _connections.end())
					continue;
				if (pollFds[i].revents & POLLHUP)
				{
					if (_connections[pollFds[i].fd]->hasPendingWrite())
						_connections[pollFds[i].fd]->setState(CONN_CLOSING);
					else
						_closeConnection(pollFds[i].fd);
				}
			}
			_checkCgiProcesses();
			_checkTimeouts();
		}
	}
	catch (...)
	{
		_running = false;
	}
	return (_running ? 0 : 1);
}

/*
 * Accepts one client after its listener reports POLLIN.
 * The new Connection owns the returned non-blocking client descriptor.
 */
void	Server::_acceptNew(int listenFd)
{
	int									clientFd;
	Connection							*connection;
	std::map<int, std::size_t>::iterator	config;
	std::string							remoteAddr;

	config = _listenerConfigs.find(listenFd);
	if (config == _listenerConfigs.end())
		return ;
	for (std::size_t i = 0; i < _listeners.size(); ++i)
	{
		if (_listeners[i]->fd() != listenFd)
			continue;
		clientFd = _listeners[i]->acceptClient(remoteAddr);
		if (clientFd < 0)
		{
			_acceptPausedUntil = std::time(0) + 1;
			return ;
		}
		if (_connections.size() >= MAX_CONNECTIONS)
		{
			::close(clientFd);
			return ;
		}
		connection = NULL;
		try
		{
			connection = new Connection(clientFd, config->second, remoteAddr);
			_connections[clientFd] = connection;
		}
		catch (...)
		{
			delete connection;
			if (connection == NULL)
				::close(clientFd);
		}
		return ;
	}
}

/*
 * Receives one ready chunk from a client after POLLIN.
 * The temporary echo block is replaced by HTTP parsing during phase 3.
 */
void	Server::_onReadable(int fd)
{
	std::map<int, Connection *>::iterator	it;
	char									buffer[WEBSERV_RECV_CHUNK];
	ssize_t									count;
	Router									router;
	HttpHandler								handler;
	RouteMatch								route;
	std::string								scriptPath;
	std::vector<std::string>				environment;
	std::vector<int>						inheritedFds;
	CgiProcess								*cgi;
	Response								response;

	it = _connections.find(fd);
	if (it == _connections.end())
		return ;
	count = ::recv(fd, buffer, sizeof(buffer), 0);
	if (count > 0)
	{
		if (static_cast<std::size_t>(count)
			> MAX_READ_BUFFER - it->second->readBuf().size())
		{
			_closeConnection(fd);
			return ;
		}
		try
		{
			it->second->readBuf().append(buffer, static_cast<std::size_t>(count));
			it->second->touch();
			if (_echoWorkaround)
			{
				// ============================================================
				// TODO: This is a workaround to allow me keep going on, must be
				// changed/improved before the project end.
				// WHY (kebris-c): proves non-blocking recv/send + multi-port while
				// Request/HttpHandler are stubs (not HTTP; transport only).
				// YOU (kmarrero): when Config::load works, main must stop enabling
				// echo; then DELETE this whole branch so the path below
				// (parse -> router -> prepareCgi/handle) always runs.
				// RESPECT: never add recv/send here from your side; leave readiness
				// gating, partial send, timeouts, and CGI poll wiring untouched.
				// ============================================================
				it->second->writeBuf().append(it->second->readBuf());
				it->second->readBuf().clear();
				return ;
			}
			if (!it->second->request().parse(it->second->readBuf()))
				return ;
			if (it->second->request().state() == REQ_ERROR)
				response = Response::makeError(
						it->second->request().errorCode());
			else
			{
				if (it->second->serverIndex() >= _configs.size())
					throw 0;
				route = router.match(_configs[it->second->serverIndex()],
						it->second->request());
				if (handler.prepareCgi(it->second->request(), route,
						it->second->remoteAddr(), scriptPath, environment))
				{
					if (route.location == NULL)
						throw 0;
					for (std::size_t i = 0; i < _listeners.size(); ++i)
						inheritedFds.push_back(_listeners[i]->fd());
					for (std::map<int, Connection *>::iterator current
							= _connections.begin();
						 current != _connections.end(); ++current)
					{
						inheritedFds.push_back(current->first);
						if (current->second->cgi() != NULL)
						{
							inheritedFds.push_back(
								current->second->cgi()->stdinFd());
							inheritedFds.push_back(
								current->second->cgi()->stdoutFd());
						}
					}
					cgi = new CgiProcess();
					it->second->attachCgi(cgi);
					if (!cgi->start(*route.location, scriptPath, environment,
							it->second->request().body(), inheritedFds))
					{
						it->second->clearCgi();
						response = Response::makeError(500);
					}
					else
					{
						it->second->setState(CONN_PROCESSING);
						return ;
					}
				}
				else
					response = handler.handle(it->second->request(), route);
			}
			it->second->clearWrite();
			it->second->writeBuf() = response.raw();
			it->second->setState(CONN_WRITING_RESPONSE);
		}
		catch (...)
		{
			_closeConnection(fd);
		}
		return ;
	}
	if (count == 0 && it->second->hasPendingWrite())
		it->second->setState(CONN_CLOSING);
	else
		_closeConnection(fd);
}

/*
 * Sends one ready portion of queued output after POLLOUT.
 * It advances an offset so a short send never loses response bytes.
 */
void	Server::_onWritable(int fd)
{
	std::map<int, Connection *>::iterator	it;
	std::size_t								remaining;
	ssize_t									count;

	it = _connections.find(fd);
	if (it == _connections.end() || !it->second->hasPendingWrite())
		return ;
	remaining = it->second->writeBuf().size() - it->second->bytesSent();
	count = ::send(fd, it->second->writeBuf().c_str()
			+ it->second->bytesSent(), remaining, 0);
	if (count <= 0)
	{
		_closeConnection(fd);
		return ;
	}
	it->second->addBytesSent(static_cast<std::size_t>(count));
	it->second->touch();
	if (!it->second->hasPendingWrite())
	{
		it->second->clearWrite();
		if (it->second->state() == CONN_CLOSING
			|| it->second->state() == CONN_WRITING_RESPONSE)
			_closeConnection(fd);
	}
}

/*
 * Dispatches one ready CGI pipe event found in the shared poll set.
 * Returns true when fd belongs to a CGI process rather than a client socket.
 */
bool	Server::_onCgiEvent(int fd, short revents)
{
	CgiProcess	*cgi;
	int			clientFd;

	for (std::map<int, Connection *>::iterator it = _connections.begin();
		 it != _connections.end(); ++it)
	{
		cgi = it->second->cgi();
		if (cgi == NULL || (cgi->stdinFd() != fd && cgi->stdoutFd() != fd))
			continue;
		clientFd = it->first;
		if (revents & (POLLERR | POLLNVAL))
		{
			cgi->fail();
			return (true);
		}
		if (fd == cgi->stdinFd() && (revents & POLLOUT))
		{
			cgi->onPipeWritable();
			if (_connections.find(clientFd) != _connections.end())
				_connections[clientFd]->touch();
		}
		if (fd == cgi->stdoutFd() && (revents & (POLLIN | POLLHUP)))
		{
			cgi->onPipeReadable();
			if (_connections.find(clientFd) != _connections.end())
				_connections[clientFd]->touch();
		}
		return (true);
	}
	return (false);
}

/*
 * Reaps active CGI children and queues their HTTP result for client POLLOUT.
 * It runs once per event-loop iteration and never waits for child termination.
 */
void	Server::_checkCgiProcesses()
{
	std::map<int, Connection *>::iterator	it;
	Connection								*connection;
	CgiProcess								*cgi;
	HttpHandler								handler;
	Response								response;
	int										fd;

	it = _connections.begin();
	while (it != _connections.end())
	{
		fd = it->first;
		connection = it->second;
		++it;
		if (connection->cgi() == NULL)
			continue;
		cgi = connection->cgi();
		cgi->tryReap();
		if (connection->state() == CONN_CLOSING)
		{
			if (cgi->childActive())
			{
				if (cgi->state() != CGI_TERMINATING)
					cgi->terminate();
				continue;
			}
			connection->clearCgi();
			_closeConnection(fd);
			continue;
		}
		if (connection->state() != CONN_PROCESSING)
		{
			if (!cgi->childActive()
				&& (cgi->state() == CGI_FAILED
					|| cgi->state() == CGI_TERMINATING))
				connection->clearCgi();
			continue;
		}
		if (cgi->state() != CGI_DONE && cgi->state() != CGI_FAILED)
			continue;
		try
		{
			if (cgi->state() == CGI_DONE)
			{
				response = handler.parseCgiOutput(cgi->output());
				connection->clearCgi();
			}
			else
			{
				response = Response::makeError(500);
				if (cgi->childActive())
					cgi->terminate();
				else
					connection->clearCgi();
			}
			connection->clearWrite();
			connection->writeBuf() = response.raw();
			connection->setState(CONN_WRITING_RESPONSE);
			connection->touch();
		}
		catch (...)
		{
			_closeConnection(fd);
		}
	}
}

/* Deletes one connection; its destructor closes the client descriptor. */
void	Server::_closeConnection(int fd)
{
	std::map<int, Connection *>::iterator it = _connections.find(fd);
	if (it != _connections.end())
	{
		if (it->second->cgi() != NULL && it->second->cgi()->childActive())
		{
			it->second->shutdownClient();
			it->second->cgi()->terminate();
			it->second->setState(CONN_CLOSING);
			return ;
		}
		delete it->second;
		_connections.erase(it);
	}
}

/* Closes clients that made no successful I/O before the timeout. */
void	Server::_checkTimeouts()
{
	std::vector<int>	expired;
	std::time_t		now;

	now = std::time(0);
	try
	{
		for (std::map<int, Connection *>::iterator it = _connections.begin();
			 it != _connections.end(); ++it)
		{
			if (it->second->state() == CONN_CLOSING)
				continue;
			if (it->second->timedOut(now, WEBSERV_CLIENT_TIMEOUT_SEC)
				|| ((it->second->state() == CONN_PROCESSING
						|| (it->second->state() == CONN_READING_REQUEST
							&& it->second->request().state() != REQ_BODY))
					&& it->second->phaseTimedOut(
						now, MAX_PHASE_DURATION_SEC)))
				expired.push_back(it->first);
		}
	}
	catch (...)
	{
		return ;
	}
	for (std::size_t i = 0; i < expired.size(); ++i)
		_closeConnection(expired[i]);
}
