/* **************************************************************************
 * Webserv.hpp — shared umbrella header
 * OWNER: both
 * GOAL: Common includes, forward declarations, project-wide constants.
 * LEARN: Prefer C++ headers (<cstring> over <string.h>). C++98 only. No Boost.
 * ============================================================================
 * Keep this light. Do not dump the whole STL here forever — only what every
 * translation unit truly needs, or split into smaller headers as the code grows.
 * ============================================================================ */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <netdb.h>
#include <poll.h> /* or epoll — choose one multiplexing API and stick to it */

/* Default path if argv[1] is missing (subject allows a default path). */
#ifndef WEBSERV_DEFAULT_CONF
# define WEBSERV_DEFAULT_CONF "configs/default.conf"
#endif

/* Soft defaults — override from config when implemented. */
#ifndef WEBSERV_RECV_CHUNK
# define WEBSERV_RECV_CHUNK 4096
#endif

#ifndef WEBSERV_CLIENT_TIMEOUT_SEC
# define WEBSERV_CLIENT_TIMEOUT_SEC 60
#endif

/* Forward declarations — flesh out in owning headers. */
class Config;
class Server;
class Connection;
class Request;
class Response;

#endif /* WEBSERV_HPP */
