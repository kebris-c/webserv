/* **************************************************************************
 * CgiProcess.cpp
 * OWNER: kebris-c (start/pipes/poll callbacks) + kmarrero (buildEnv + output parse)
 * ************************************************************************** */

#include "CgiProcess.hpp"

CgiProcess::CgiProcess()
	: _pid(-1), _toChild(-1), _fromChild(-1), _state(CGI_INIT), _inputPos(0)
{
}

CgiProcess::~CgiProcess()
{
	if (_toChild >= 0)
		::close(_toChild);
	if (_fromChild >= 0)
		::close(_fromChild);
	/* TODO(kebris-c): if still running, kill/wait carefully during shutdown */
}

std::vector<std::string>	CgiProcess::buildEnv(const Request &req,
													const ServerConfig &srv,
													const LocationConfig &loc,
													const std::string &scriptPath)
{
	/*
	 * TODO(kmarrero): RFC 3875 — return KEY=VALUE strings for execve envp
	 * Minimum useful set:
	 *   REQUEST_METHOD, SCRIPT_FILENAME/SCRIPT_NAME, PATH_INFO, QUERY_STRING,
	 *   CONTENT_LENGTH, CONTENT_TYPE, SERVER_PROTOCOL, SERVER_NAME, SERVER_PORT,
	 *   REMOTE_ADDR, GATEWAY_INTERFACE=CGI/1.1, REDIRECT_STATUS=200 (php-cgi)
	 */
	(void)req;
	(void)srv;
	(void)loc;
	(void)scriptPath;
	return (std::vector<std::string>());
}

bool	CgiProcess::start(const LocationConfig &loc,
							const std::string &scriptPath,
							const std::vector<std::string> &env,
							const std::string &body)
{
	/*
	 * TODO(kebris-c): see PSEUDOCODE in CgiProcess.hpp
	 * INVESTIGATE: pipe + fork + dup2 + execve; non-blocking parent fds;
	 *              registering those fds in Server's poll set (critical)
	 */
	(void)loc;
	(void)scriptPath;
	(void)env;
	_input = body;
	_inputPos = 0;
	_state = CGI_FAILED;
	return (false);
}

int	CgiProcess::stdinFd() const { return (_toChild); }
int	CgiProcess::stdoutFd() const { return (_fromChild); }
CgiState	CgiProcess::state() const { return (_state); }
const std::string	&CgiProcess::output() const { return (_output); }

void	CgiProcess::onPipeWritable()
{
	/* TODO(kebris-c): write next chunk of _input; close stdin when done */
}

void	CgiProcess::onPipeReadable()
{
	/* TODO(kebris-c): read into _output; detect EOF */
}

void	CgiProcess::tryReap()
{
	/* TODO(kebris-c): waitpid(_pid, &st, WNOHANG); transition to DONE/FAILED */
}
