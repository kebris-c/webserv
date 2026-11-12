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
