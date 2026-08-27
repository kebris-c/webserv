/* **************************************************************************
 * CgiProcess.hpp
 * OWNER: kebris-c (process/pipes/poll integration)
 *        kmarrero (env vars, working directory, interpreting CGI output)
 * GOAL: Execute one CGI for a request; feed stdin; collect stdout asynchronously.
 * LEARN:
 *   - RFC 3875 environment variables (REQUEST_METHOD, SCRIPT_NAME, PATH_INFO,
 *     QUERY_STRING, CONTENT_LENGTH, CONTENT_TYPE, SERVER_PROTOCOL, ...)
 *   - fork + dup2 + execve; only use fork for CGI (subject)
 *   - Unchunked body on CGI stdin; EOF marks end
 *   - If CGI omits Content-Length, EOF ends output
 *   - chdir to correct directory for relative paths
 *   - Pipes are I/O that can block => must be non-blocking + in the SAME poll loop
 * ************************************************************************** */

#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include "Webserv.hpp"
#include "Config.hpp"
#include "Request.hpp"

enum CgiState {
	CGI_INIT,
	CGI_RUNNING,
	CGI_DONE,
	CGI_FAILED,
	CGI_TERMINATING
};

class CgiProcess {
public:
	/* Creates an inactive CGI process with no open pipes. */
	CgiProcess();
	/* Closes pipes and terminates a child still owned during shutdown. */
	~CgiProcess();

	/* kmarrero: build env from Request + Location + ServerConfig */
	static std::vector<std::string>	buildEnv(const Request &req,
												const ServerConfig &srv,
												const LocationConfig &loc,
												const std::string &scriptPath);

	/* Spawns one CGI and exposes parent pipe ends for Server's poll loop. */
	bool	start(const LocationConfig &loc,
					const std::string &scriptPath,
					const std::vector<std::string> &env,
					const std::string &body,
					const std::vector<int> &inheritedFds);

	/* Returns the parent-to-CGI fd, or -1 after request-body EOF. */
	int		stdinFd() const;
	/* Returns the CGI-to-parent fd, or -1 after response EOF. */
	int		stdoutFd() const;
	/* Returns lifecycle state used when Server builds poll interests. */
	CgiState	state() const;
	/* Reports whether this object still owns an unreaped child pid. */
	bool		childActive() const;
	/* Returns raw CGI stdout for kmarrero's response parser. */
	const std::string	&output() const;

	/* Writes one ready body chunk after Server reports POLLOUT. */
	void	onPipeWritable();
	/* Reads one ready output chunk after Server reports POLLIN. */
	void	onPipeReadable();
	/* Reaps the child without blocking; Server calls it each loop. */
	void	tryReap();
	/* Marks pipe transport failed without blocking on child termination. */
	void	fail();
	/* Sends SIGKILL and leaves non-blocking reaping to Server. */
	void	terminate();

	/*
	 * PSEUDOCODE — split ownership
	 *
	 * kmarrero:
	 *   env = buildEnv(...); ensure PATH_TRANSLATED / SCRIPT_FILENAME as needed
	 *
	 * kebris-c:
	 *   pipe(in), pipe(out); setNonBlocking both parent ends
	 *   pid = fork()
	 *   child: dup2 in[0]->stdin, out[1]->stdout; close extras;
	 *          chdir(cgi cwd); execve(cgiPass, argv, envp)
	 *   parent: close child ends; write body to in[1] when POLLOUT;
	 *           read out[0] when POLLIN; on EOF + waitpid -> CGI_DONE
	 *
	 * kmarrero (after DONE):
	 *   parse CGI headers (until blank line) + body -> HTTP Response
	 *   default status 200 if CGI did not send Status:
	 */

private:
	pid_t		_pid;
	int			_toChild;		/* parent -> CGI stdin */
	int			_fromChild;		/* CGI stdout -> parent */
	CgiState	_state;
	std::string	_input;			/* remaining body to write */
	std::size_t	_inputPos;
	std::string	_output;

	/* Closes both parent pipe ends and invalidates their descriptors. */
	void		_closePipes();

	CgiProcess(const CgiProcess &);
	CgiProcess	&operator=(const CgiProcess &);
};

#endif /* CGI_PROCESS_HPP */
