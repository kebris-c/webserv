/* **************************************************************************
 * Config.cpp
 * OWNER: kmarrero
 * GOAL: Implement nginx-inspired parser. Skeleton only.
 * LEARN: Start by parsing the sample configs/default.conf by hand on paper,
 *        then tokenize braces and directives.
 * ************************************************************************** */

#include "Config.hpp"

Config::Config()
{}

Config::~Config() {}

void	Config::addTransitions(StateMachine& stateMachine)
{
	stateMachine.addTransition(START, BALANCE, &Parser::balance);
	stateMachine.addTransition(BLOCK_KEYWORD, BLOCK_KEYWORD_EVENT, &Parser::blockKeyWord);
	stateMachine.addTransition(LBRACET, BEGIN_BLOCK, &Parser::insideBlock);
	stateMachine.addTransition(RBRACET, CLOSE_BLOCK, &Parser::outsideBlock);
	stateMachine.addTransition(DIRECTIVE, DIRECTIVE_EVENT, &Parser::keyword);
}

void	Config::setServerConfig(std::vector<ServerConfig>& server)
{
	this->_servers = server;
}

bool	Config::load(const std::string &path)
{
	ParserContext				ctx;
	std::ifstream				file;
	Lexer						lexer;
	ParserState					state;
	ParserEvent					event;
	Parser						parser;
	StateMachine				stateMachine(START);
	std::vector<ServerConfig>	server;

	ctx.bracet = 0;
	ctx.lineNumber = 0;
	if (lexer.obtainInfile(file, path))
		return (false);
	if (lexer.checkFileContent(file))
		return (false);
	if (lexer.tokenVectorization(file))
		return (false);
	ctx.tokens = lexer.getTokens();
	addTransitions(stateMachine);
	state = stateMachine.getCurrentState();
	while (state != END)
	{
		event = stateMachine.getNextEvent(state);
		stateMachine.handle(ctx, parser, event);
		state = stateMachine.getCurrentState();
		if (state == SINTAX_ERROR || state == ERROR)
			break ;
	}
	if (ctx.error != "")
		return (false);
	server = parser.getServer();
	setServerConfig(parser.getServer());
	return (true);
}

const std::vector<ServerConfig>	&Config::servers() const
{
	return (_servers);
}
