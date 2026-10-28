/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiProcess.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 17:35:10 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/28 17:35:13 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiProcess.hpp"

namespace {

const std::size_t	MAX_CGI_OUTPUT = 16 * 1024 * 1024;

}

/* Creates an inactive CGI owner with no child or pipe descriptors. */
CgiProcess::CgiProcess()
	: _pid(-1), _toChild(-1), _fromChild(-1), _state(CGI_INIT), _inputPos(0)
{
}

/* Closes parent pipe ends and stops a child still running during shutdown. */
CgiProcess::~CgiProcess()
{
	_closePipes();
	if (_pid > 0)
		::kill(_pid, SIGKILL);
}

/*
 * Builds CGI environment data for a request whose URL path ends at the script.
 * scriptPath must be absolute; prepareCgi adds the client's REMOTE_ADDR.
 */
std::vector<std::string>	CgiProcess::buildEnv(const Request &req,
													const ServerConfig &srv,
													const LocationConfig &loc,
													const std::string &scriptPath)
{
	std::vector<std::string>	cgiEnv;
	std::ostringstream		length;
	std::ostringstream		port;

	(void)loc;
	if (scriptPath.empty() || scriptPath[0] != '/')
		throw std::invalid_argument("CGI scriptPath must be absolute");
	length << req.body().size();
	port << srv.port;
	cgiEnv.push_back("REQUEST_METHOD=" + req.method());
	cgiEnv.push_back("QUERY_STRING=" + req.query());
	cgiEnv.push_back("SERVER_PROTOCOL=" + req.version());
	cgiEnv.push_back("SERVER_PORT=" + port.str());
	cgiEnv.push_back("SERVER_NAME="
		+ (srv.serverName.empty() ? srv.host : srv.serverName));
	cgiEnv.push_back("GATEWAY_INTERFACE=CGI/1.1");
	cgiEnv.push_back("SCRIPT_FILENAME=" + scriptPath);
	cgiEnv.push_back("SCRIPT_NAME=" + req.target());
	cgiEnv.push_back("CONTENT_LENGTH=" + length.str());
	if (!req.header("Content-Type").empty())
		cgiEnv.push_back("CONTENT_TYPE=" + req.header("Content-Type"));
	cgiEnv.push_back("REDIRECT_STATUS=200");
	cgiEnv.push_back("PATH_INFO=");
	return (cgiEnv);
}

/*
 * Creates CGI stdin/stdout pipes and executes one configured interpreter.
 * HttpHandler will call it; Server must register the returned parent fds.
 */
bool	CgiProcess::start(const LocationConfig &loc,
							const std::string &scriptPath,
							const std::vector<std::string> &env,
							const std::string &body,
							const std::vector<int> &inheritedFds)
{
	int					inputPipe[2];
	int					outputPipe[2];
	std::vector<char *>	environment;
	char				*arguments[3];
	std::string			directory;
	std::string			scriptName;
	std::size_t			slash;

	if (_pid > 0 || _state == CGI_RUNNING
		|| loc.cgiPass.empty() || scriptPath.empty())
		return (false);
	if (_toChild >= 0)
		::close(_toChild);
	if (_fromChild >= 0)
		::close(_fromChild);
	_toChild = -1;
	_fromChild = -1;
	_pid = -1;
	_output.clear();
	_input = body;
	_inputPos = 0;
	_state = CGI_INIT;
	try
	{
		environment.reserve(env.size() + 1);
		for (std::size_t i = 0; i < env.size(); ++i)
			environment.push_back(const_cast<char *>(env[i].c_str()));
		environment.push_back(NULL);
	}
	catch (...)
	{
		_state = CGI_FAILED;
		return (false);
	}
	slash = scriptPath.find_last_of('/');
	if (slash == std::string::npos)
	{
		directory = ".";
		scriptName = scriptPath;
	}
	else
	{
		directory = slash == 0 ? "/" : scriptPath.substr(0, slash);
		scriptName = scriptPath.substr(slash + 1);
	}
	arguments[0] = const_cast<char *>(loc.cgiPass.c_str());
	arguments[1] = const_cast<char *>(scriptName.c_str());
	arguments[2] = NULL;
	if (::pipe(inputPipe) < 0)
	{
		_state = CGI_FAILED;
		return (false);
	}
	if (::pipe(outputPipe) < 0)
	{
		::close(inputPipe[0]);
		::close(inputPipe[1]);
		_state = CGI_FAILED;
		return (false);
	}
	if (::fcntl(inputPipe[1], F_SETFL, O_NONBLOCK) < 0
		|| ::fcntl(outputPipe[0], F_SETFL, O_NONBLOCK) < 0)
	{
		::close(inputPipe[0]);
		::close(inputPipe[1]);
		::close(outputPipe[0]);
		::close(outputPipe[1]);
		_state = CGI_FAILED;
		return (false);
	}
	std::cout.flush();
	std::cerr.flush();
	_pid = ::fork();
	if (_pid < 0)
	{
		::close(inputPipe[0]);
		::close(inputPipe[1]);
		::close(outputPipe[0]);
		::close(outputPipe[1]);
		_state = CGI_FAILED;
		return (false);
	}
	if (_pid == 0)
	{
		::signal(SIGPIPE, SIG_DFL);
		::close(inputPipe[1]);
		::close(outputPipe[0]);
		for (std::size_t i = 0; i < inheritedFds.size(); ++i)
		{
			if (inheritedFds[i] >= 0)
				::close(inheritedFds[i]);
		}
		if ((inputPipe[0] != STDIN_FILENO
				&& ::dup2(inputPipe[0], STDIN_FILENO) < 0)
			|| (outputPipe[1] != STDOUT_FILENO
				&& ::dup2(outputPipe[1], STDOUT_FILENO) < 0))
			std::exit(126);
		if (inputPipe[0] != STDIN_FILENO)
			::close(inputPipe[0]);
		if (outputPipe[1] != STDOUT_FILENO)
			::close(outputPipe[1]);
		if (::chdir(directory.c_str()) < 0)
			std::exit(126);
		::execve(loc.cgiPass.c_str(), arguments, &environment[0]);
		std::exit(127);
	}
	::close(inputPipe[0]);
	::close(outputPipe[1]);
	_toChild = inputPipe[1];
	_fromChild = outputPipe[0];
	if (_input.empty())
	{
		::close(_toChild);
		_toChild = -1;
	}
	_state = CGI_RUNNING;
	return (true);
}

/* Returns the parent pipe used to send the unchunked request body. */
int	CgiProcess::stdinFd() const { return (_toChild); }

/* Returns the parent pipe used to collect raw CGI output. */
int	CgiProcess::stdoutFd() const { return (_fromChild); }

/* Returns the process lifecycle state used by Server. */
CgiState	CgiProcess::state() const { return (_state); }

/* Reports whether waitpid has not yet reaped the owned child. */
bool	CgiProcess::childActive() const { return (_pid > 0); }

/* Returns CGI stdout after EOF for kmarrero's HTTP conversion. */
const std::string	&CgiProcess::output() const { return (_output); }

/* Writes one body chunk after the single poll loop reports POLLOUT. */
void	CgiProcess::onPipeWritable()
{
	ssize_t	count;

	if (_state != CGI_RUNNING || _toChild < 0)
		return ;
	count = ::write(_toChild, _input.c_str() + _inputPos,
			_input.size() - _inputPos);
	if (count <= 0)
	{
		fail();
		return ;
	}
	_inputPos += static_cast<std::size_t>(count);
	if (_inputPos == _input.size())
	{
		::close(_toChild);
		_toChild = -1;
		_input.clear();
		_inputPos = 0;
	}
}

/* Reads one output chunk after the single poll loop reports POLLIN. */
void	CgiProcess::onPipeReadable()
{
	char	buffer[WEBSERV_RECV_CHUNK];
	ssize_t	count;

	if (_state != CGI_RUNNING || _fromChild < 0)
		return ;
	count = ::read(_fromChild, buffer, sizeof(buffer));
	if (count > 0)
	{
		if (static_cast<std::size_t>(count)
			> MAX_CGI_OUTPUT - _output.size())
		{
			fail();
			return ;
		}
		try
		{
			_output.append(buffer, static_cast<std::size_t>(count));
		}
		catch (...)
		{
			fail();
		}
		return ;
	}
	::close(_fromChild);
	_fromChild = -1;
	if (count < 0)
		fail();
	else if (_pid < 0)
		_state = CGI_DONE;
}

/* Reaps the CGI without blocking and records successful or failed exit. */
void	CgiProcess::tryReap()
{
	int		status;
	pid_t	result;

	if (_pid <= 0 || (_state != CGI_RUNNING && _state != CGI_FAILED
			&& _state != CGI_TERMINATING))
		return ;
	result = ::waitpid(_pid, &status, WNOHANG);
	if (result == 0)
		return ;
	if (result < 0)
	{
		if (errno == EINTR)
			return ;
		_pid = -1;
		_state = CGI_FAILED;
		return ;
	}
	_pid = -1;
	if (_state == CGI_TERMINATING
		|| !WIFEXITED(status) || WEXITSTATUS(status) != 0)
		_state = CGI_FAILED;
	else if (_fromChild < 0)
		_state = CGI_DONE;
}

/* Closes pipe transport and records a failure without waiting for the child. */
void	CgiProcess::fail()
{
	_closePipes();
	_state = CGI_FAILED;
}

/* Kills an active child; Server continues calling tryReap() with WNOHANG. */
void	CgiProcess::terminate()
{
	_closePipes();
	if (_pid <= 0)
		return ;
	::kill(_pid, SIGKILL);
	_state = CGI_TERMINATING;
}

/* Closes both parent pipe ends once and invalidates their descriptors. */
void	CgiProcess::_closePipes()
{
	if (_toChild >= 0)
		::close(_toChild);
	if (_fromChild >= 0)
		::close(_fromChild);
	_toChild = -1;
	_fromChild = -1;
}
