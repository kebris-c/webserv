/* ************************************************************************** */
/* Config.hpp
 * OWNER: kmarrero
 * GOAL: Parse nginx-inspired config into ServerConfig / Location structures.
 * LEARN:
 *   - nginx `server` / `location` mental model
 *   - tokenization vs recursive descent for braces
 *   - subject IV.3 directives list (listen, error_page, client_max_body_size,
 *     methods, return/redirect, root, autoindex, index, upload_store, cgi)
 * NOTES:
 *   - Virtual hosts are optional/out of scope; server_name can wait.
 *   - No regex locations required.
 *   - Provide configs that prove every feature at evaluation.
 * ************************************************************************** */

#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "Webserv.hpp"

struct LocationConfig {
	std::string					path;			/* URL prefix, e.g. /upload */
	std::vector<std::string>	allowedMethods;	/* GET POST DELETE */
	std::string					root;			/* filesystem root for this location */
	std::string					index;			/* default file for directories */
	bool						autoindex;		/* directory listing on/off */
	std::string					redirect;		/* empty = none; else target URL/path */
	int							redirectCode;	/* 301/302/... */
	std::string					uploadStore;	/* directory for uploads */
	std::string					cgiExtension;	/* e.g. .py */
	std::string					cgiPass;		/* interpreter / cgi binary path */

	LocationConfig();
};

struct ServerConfig {
	std::string					host;			/* interface, e.g. 0.0.0.0 or 127.0.0.1 */
	int							port;			/* listen port */
	std::string					serverName;		/* optional */
	std::map<int, std::string>	errorPages;		/* status -> file path */
	std::size_t					clientMaxBodySize; /* bytes */
	std::vector<LocationConfig>	locations;

	ServerConfig();
};

class Config {
	public:
			Config();
			~Config();

			/* Parse path; throw std::runtime_error or return false on hard failure. */
			bool	load(const std::string &path);

			const std::vector<ServerConfig>	&servers() const;

	private:
			std::vector<ServerConfig>	_servers;
			

			/* TODO(kmarrero): lexer + parseServer + parseLocation helpers. */
};

#endif /* CONFIG_HPP */
