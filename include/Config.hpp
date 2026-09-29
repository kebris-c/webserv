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

# include "Lexer.hpp"
# include "ConfParser.hpp"
# include "Webserv.hpp"
# include "ConfStateMachine.hpp"

class	Config
{
	private:
		std::vector<ServerConfig>	_servers;
		void	addTransitions(StateMachine& stateMachine);
	public:
		Config();
		~Config();
		bool							load(const std::string &path);
		const std::vector<ServerConfig>	&servers() const;
		void							setServerConfig(std::vector<ServerConfig>& server);
};

#endif /* CONFIG_HPP */
